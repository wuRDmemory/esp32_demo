import time
import jwt
import requests

def generate_token(apikey: str, exp_seconds: int):
    try:
        id, secret = apikey.split(".")
    except Exception as e:
        raise Exception("invalid apikey", e)
 
    payload = {
        "api_key": id,
        "exp": int(round(time.time() * 1000)) + exp_seconds * 1000,
        "timestamp": int(round(time.time() * 1000)),
    }

    
    return jwt.encode(
        payload,
        secret,
        algorithm="HS256",
        headers={"alg": "HS256", "sign_type": "SIGN"},
    )

api_key = "756abc5bfdcac6a49dbad10e10ff5d17.nf4BXdXGtIIVewv9"
host = "https://open.bigmodel.cn/api/paas/v4/chat/completions"

token = generate_token(api_key, 100)
print(token)

response = requests.post(
    host,
    headers={
        "Authorization": "Bearer " + token,
        "Content-Type": "application/json",
    },
    json={
        "messages": [
            {"role": "user", "content": "作为一名营销专家，请为我的产品创作一个吸引人的slogan"},
            {"role": "assistant", "content": "当然，为了创作一个吸引人的slogan，请告诉我一些关于您产品的信息"},
            {"role": "user", "content": "智谱AI开放平台"},
            {"role": "assistant", "content": "智启未来，谱绘无限一智谱AI，让创新触手可及!"},
            {"role": "user", "content": "创造一个更精准、吸引人的slogan"}
        ],
        "model": "glm-3-turbo",
        "stream": "false"
    },
    stream=True,
)

for chunk in response.iter_content(chunk_size=128):
    print(chunk)
# print(response.text)

