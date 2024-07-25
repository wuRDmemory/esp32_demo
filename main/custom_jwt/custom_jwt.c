#include "custom_jwt.h"
#include "base64url.h"
#include "sha256.h"
#include "mbedtls/md.h"
#include "string.h"

#define SHA256_HASH 32

static uint8_t *_secret;
static char *_alg;
static char *_typ;
static char _staticAllocation = 0;
static char _memoryAllocationDone = 0;

static size_t _maxHeadLen;
static size_t _maxPayloadLen;
static size_t _maxSigLen;

static char *_header = NULL;
static char *_payload = NULL;
static char *_signature = NULL;
static size_t _headerLength;
static size_t _payloadLength;
static size_t _signatureLength;
static size_t _b64HeaderLen;
static size_t _b64PayloadLen;
static size_t _b64SigLen;
static size_t _maxOutputLen;

char *jwt_out = NULL;
size_t jwt_outputLength;

void setCustomJWT(const char *secret, size_t maxPayloadLen,
                  size_t maxHeadLen, size_t maxSigLen,
                  char *alg, char *typ) {
    _secret = (uint8_t *)malloc((strlen(secret)+1) * sizeof(uint8_t));
    _alg = (char *)malloc((strlen(alg)+1) * sizeof(char));
    _typ = (char *)malloc((strlen(typ)+1) * sizeof(char));
    memcpy(_secret, secret, (strlen(secret)+1));
    memcpy(_alg, alg, (strlen(alg)+1));
    memcpy(_typ, typ, (strlen(typ)+1));
    _maxHeadLen = maxHeadLen;
    _maxPayloadLen = maxPayloadLen;
    _maxSigLen = maxSigLen;
    _staticAllocation = 0;
}

int8_t allocateJWTMemory() {
    if(_staticAllocation)
        return 0;

    _b64HeaderLen = (size_t)(4.0 * (_maxHeadLen / 3.0)) + 5;
    _b64PayloadLen = (size_t)(4.0 * (_maxPayloadLen / 3.0)) + 5;
    _b64SigLen = (size_t)(4.0 * (_maxSigLen / 3.0)) + 5;
    _maxOutputLen = (size_t)(_b64HeaderLen + _b64PayloadLen + _b64SigLen + 4);

    _header    = (char *)malloc(_b64HeaderLen * sizeof(char));
    _payload   = (char *)malloc(_b64PayloadLen * sizeof(char));
    _signature = (char *)malloc(_b64SigLen * sizeof(char));
    jwt_out = (char *)malloc(_maxOutputLen * sizeof(char));

    _memoryAllocationDone = 1;
    return 1;
}

void generateSignature(char *output, size_t *outputLen, void *secret, size_t secretLen, void *data, size_t dataLen) {
    uint8_t hashed[SHA256_HASH];
    memset(hashed, 0, SHA256_HASH);
    // mbedtls_md_context_t ctx;
    // mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    // mbedtls_md_init(&ctx);
    // mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    // mbedtls_md_hmac_starts(&ctx, (const unsigned char *) secret, secretLen);
    // mbedtls_md_hmac_update(&ctx, (const unsigned char *) data, dataLen);
    // mbedtls_md_hmac_finish(&ctx, hashed);
    // mbedtls_md_free(&ctx);
    hmac_sha256_vector((const uint8_t *)secret, secretLen, 1, (const uint8_t *)&data, &dataLen, hashed);
    // hmac<SHA256>(hashed, SHA256_HASH, secret, secretLen, data, dataLen);
    base64urlEncode(hashed, SHA256_HASH, output, outputLen);
}

uint8_t encodeJWT(char *payloadJSON) {
    if(!_memoryAllocationDone && !_staticAllocation) {
        return 0;
    }

    if(!_staticAllocation) {
        memset(_header,    0, sizeof(char) * _b64HeaderLen);
        memset(_payload,   0, sizeof(char) * _b64PayloadLen);
        memset(_signature, 0, sizeof(char) * _b64SigLen);
        memset(jwt_out,    0, sizeof(char) * _maxOutputLen);
    } else {
        memset(_header,    0, sizeof(char) * _maxHeadLen);
        memset(_payload,   0, sizeof(char) * _maxPayloadLen);
        memset(_signature, 0, sizeof(char) * _maxSigLen);
        memset(jwt_out,    0, sizeof(char) * _maxOutputLen);
    }

    // Base64url encode the header
    char headerJSON[_maxHeadLen];
    sprintf(headerJSON, "{\"alg\": \"%s\",\"sign_type\": \"%s\"}", _alg, _typ);
    base64urlEncode(headerJSON, strlen(headerJSON), _header, &_headerLength);
    
    // Base64url encode the payload
    base64urlEncode(payloadJSON, strlen(payloadJSON), _payload, &_payloadLength);
    
    char toHash[_payloadLength + _headerLength + 3];
    memset(toHash, 0, _payloadLength + _headerLength + 3);
    sprintf(toHash, "%s.%s", _header, _payload);

    // Generate the signature
    if (_signature != NULL) {
        generateSignature(_signature, &_signatureLength, _secret, strlen((char *)_secret), toHash, strlen(toHash));
    }
    
    sprintf(jwt_out, "%s.%s", toHash, _signature);
    jwt_outputLength = strlen(jwt_out);
    return 1;
}
