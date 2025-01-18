#include "responser.h"

void Responser::deleteClient(int c)
{
    // 從 clnt_socks 中刪除對應的 socket
    auto it = find(clnt_socks.begin(), clnt_socks.end(), c);
    if (it != clnt_socks.end())
    {
        clnt_socks.erase(it); // 使用迭代器删除
    }
}
void Responser::addClient(int c)
{
    clnt_socks.push_back(c);
}

void Responser::process(int client, char *buf)
{
    // 解析 JSON 字符串
    Json::CharReaderBuilder reader;
    Json::Value parsedRoot;
    string errors;

    istringstream s(buf); // 将 buf 转换为输入流

    if (Json::parseFromStream(reader, s, &parsedRoot, &errors))
    {
        string type = parsedRoot["type"].asString();
        string content = parsedRoot["content"].asString();

        cout << "Parsed type: " << type << endl;

        if (type == "name")
        {
            cout << "Parsed name: " << content << endl;
            id_name[client] = parsedRoot["content"].asString();

            for (int j = 0; j < clnt_socks.size(); j++)
            {
                Json::Value response;
                response["type"] = "contacts";
                response["content"] = Json::Value(Json::arrayValue);
                response["getid"] = to_string(clnt_socks[j]);
                // response["getid"] = clnt_socks[j];
                for (int k = 0; k < clnt_socks.size(); k++)
                {
                    if (clnt_socks[k] != clnt_socks[j])
                    {
                        Json::Value contact;
                        contact["id"] = clnt_socks[k];
                        contact["name"] = id_name[clnt_socks[k]];
                        response["content"].append(contact);
                    }
                }
                // 将 JSON 对象转换为字符串
                Json::StreamWriterBuilder writer;
                std::string jsonString = Json::writeString(writer, response);

                // 发送 JSON 字符串
                write(clnt_socks[j], jsonString.c_str(), jsonString.size());
            }
        }
        else if (type == "msg")
        {
            std::string dst = parsedRoot["to"].asString(); // 提取 ID 部分
            int target_sock = std::stoi(dst);              // 將id轉為int

            Json::Value response;
            response["from"] = parsedRoot["from"].asString();
            response["type"] = "msg";
            response["content"] = content;

            // 将 JSON 对象转换为字符串
            Json::StreamWriterBuilder writer;
            std::string jsonString = Json::writeString(writer, response);

            write(target_sock, jsonString.c_str(), jsonString.size());
        }
    }
    else
    {
        return;
    }
}

Responser::Responser(/* args */)
{
}

Responser::~Responser()
{
}