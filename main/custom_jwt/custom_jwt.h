#ifndef __CUSTOM_JWT_H__
#define __CUSTOM_JWT_H__

#include <stdio.h>

extern char *jwt_out;
extern size_t jwt_outputLength;

/***
 * @brief Set the secret, algorithm and type for the JWT
 * @param secret The secret to use for the JWT
 * @param maxPayloadLen The maximum length of the payload
 * @param maxHeadLen The maximum length of the header
 * @param maxSigLen The maximum length of the signature
 * @param alg The algorithm to use for the JWT
 * @param typ The type of the JWT
 * @return void
*/
void setCustomJWT(const char *secret, size_t maxPayloadLen, size_t maxHeadLen, size_t maxSigLen, char *alg, char *typ);

/***
 * @brief Allocate memory for the JWT
 * @return 1 if memory allocation was successful, 0 otherwise
*/
int8_t allocateJWTMemory();

/***
 * @brief Generate the JWT header
 * @return void
*/
void generateSignature(char *output, size_t *outputLen, void *secret, size_t secretLen, void *data, size_t dataLen);

/***
 * @brief Encode the JWT
 * @param payloadJSON The payload to encode
 * @return 1 if encoding was successful, 0 otherwise
*/
uint8_t encodeJWT(char *payloadJSON);


#endif