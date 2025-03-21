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

void Responser::sendJSON(int socket, Json::Value response)
{
    // 將JSON對象轉為字串
    Json::StreamWriterBuilder writer;
    std::string jsonString = Json::writeString(writer, response);

    // 發送JSON字串
    write(socket, jsonString.c_str(), jsonString.size());
}

// 處理JSON字串
void Responser::process(int client, int len, char *buf)
{

    if (fileTranferMode)
    {
        receiveFile(len, buf);
        return;
    }

    // 解析 JSON 字符串
    Json::CharReaderBuilder reader;
    Json::Value parsedRoot;
    string errors;
    istringstream s(buf); // 將 buf 轉換成輸入流
    bool success = Json::parseFromStream(reader, s, &parsedRoot, &errors);

    if (!success)
    {
        std::cerr << "JSON parse error: " << errors << std::endl;
        std::cerr << "Received data: " << buf << std::endl; // 打印接收到的數據
        return;
    }

    string type = parsedRoot["type"].asString();
    cout << "Parsed type: " << type << endl;

    if (type == "name")
    {
        id_name[client] = parsedRoot["content"].asString();
        cout << "Parsed name: " << id_name[client] << endl;

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

            sendJSON(clnt_socks[j], response);
        }
    }
    else if (type == "msg")
    {
        Json::Value response;
        response["from"] = parsedRoot["from"].asString();
        response["type"] = "msg";
        response["content"] = parsedRoot["content"].asString();

        std::string dst = parsedRoot["to"].asString(); // 提取 ID 部分
        int target_sock = std::stoi(dst);              // 將id轉為int

        sendJSON(target_sock, response);
    }
    else if (type == "upload")
    {
        offset = 0;
        file_size = 0;
        file_name = parsedRoot["file_name"].asString();
        file_size = parsedRoot["file_size"].asInt64();
        file_type = parsedRoot["file_type"].asString(); //"file_recv"or "img_recv"

        std::string target_sock_str = parsedRoot["to"].asString();
        std::string source_sock_str = parsedRoot["from"].asString();
        if (target_sock_str != "")
        {
            file_dst = std::stoi(target_sock_str);
        }
        file_src = std::stoi(source_sock_str);

        // 發送回應
        Json::Value response;
        response["type"] = "upload_ack";

        sendJSON(file_src, response);

        fileTranferMode = true;
    }
    else if (type == "recv_ack")
    {
        file_size = parsedRoot["file_size"].asInt64();
        std::string file_name = parsedRoot["file_name"].asString();
        std::string source_sock_str = parsedRoot["from"].asString();
        int source_sock = std::stoi(source_sock_str);

        std::cout << "File " << file_name << " start sent to socket " << source_sock << std::endl;

        std::thread t1([this, source_sock, file_name]()
                       { this->sendFileThread(source_sock, file_name); });
        t1.detach();
    }
}

// 處裡client上傳檔案
void Responser::receiveFile(ssize_t bytesRead, char *buf)
{
    // 創建本地文件
    std::ofstream outFile(file_name, std::ios::app | std::ios::binary); // std::ios::app 添加到文件末尾
    if (!outFile)
    {
        std::cerr << "Failed to create file: " << file_name << std::endl;
        return;
    }
    // 接收數據並寫入文件
    if (bytesRead > 0)
    {
        outFile.write(buf, bytesRead);
        offset += bytesRead;
        // std::cout << "file chunk received" << std::endl;
    }

    if (offset >= file_size)
    {
        fileTranferMode = false;

        std::cout << "File received and saved to " << file_name << std::endl;

        notifyFileDownload();
    }
    outFile.close();
}

void Responser::sendFileThread(int source_sock, string file_name)
{

    // const int HEADER_SIZE = 1;
    const int BUFFER_SIZE = 256 * 1024;

    std::ifstream inFile(file_name, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return;
    }

    // 讀取檔案並發送給source_sock
    char buffer[BUFFER_SIZE];
    // buffer[0] = 0x2; // 檔案類型
    while (inFile.read(buffer, BUFFER_SIZE) || inFile.gcount() > 0)
    {
        ssize_t bytesSent = send(source_sock, buffer, inFile.gcount(), 0);
        if (bytesSent < 0)
        {
            std::cerr << "Failed to send file data to socket: " << source_sock << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    inFile.close();
    std::cout << "File " << file_name << " success sent to socket " << source_sock << std::endl;
}

void Responser::notifyFileDownload()
{
    // 檔案已上傳到Server，通知接收端是否接受檔案
    Json::Value response;
    response["type"] = "ask_recv";
    response["file_name"] = file_name;
    response["file_size"] = file_size;
    response["file_type"] = file_type;
    response["from"] = to_string(file_src);

    sendJSON(file_dst, response);
}

Responser::Responser(/* args */)
{
}

Responser::~Responser()
{
}