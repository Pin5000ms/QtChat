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

    istringstream s(buf); // 將 buf 轉換成輸入流
    bool success = Json::parseFromStream(reader, s, &parsedRoot, &errors);

    if (!success)
    {
        std::cerr << "JSON parse error: " << errors << std::endl;
        std::cerr << "Received data: " << buf << std::endl; // 打印接收到的數據
        return;
    }
    else if (success)
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
                // 將JSON對象轉為字串
                Json::StreamWriterBuilder writer;
                std::string jsonString = Json::writeString(writer, response);

                // 發送JSON字串
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

            // 將JSON對象轉為字串
            Json::StreamWriterBuilder writer;
            std::string jsonString = Json::writeString(writer, response);

            write(target_sock, jsonString.c_str(), jsonString.size());
        }
        else if (type == "file")
        {
            offset = 0;
            file_size = 0;
            file_name = parsedRoot["file_name"].asString();
            file_size = parsedRoot["file_size"].asInt64();

            std::string target_sock_str = parsedRoot["to"].asString();
            std::string source_sock_str = parsedRoot["from"].asString();
            if (target_sock_str != "")
            {
                file_dst = std::stoi(target_sock_str);
            }

            file_src = std::stoi(source_sock_str);

            // 發送回應
            Json::Value response;
            response["type"] = "file_ack";
            Json::StreamWriterBuilder writer;
            std::string jsonString = Json::writeString(writer, response);
            write(file_src, jsonString.c_str(), jsonString.size()); // 回應發送端

            filetranfermode = true;
        }
        else if (type == "file_recv_ack")
        {
            file_size = parsedRoot["file_size"].asInt64();
            std::string file_name = parsedRoot["file_name"].asString();
            std::string source_sock_str = parsedRoot["from"].asString();
            int source_sock = std::stoi(source_sock_str);

            std::cout << "File " << file_name << "start sent to socket " << source_sock << std::endl;
            std::thread t1([this, source_sock, file_name]()
                           { this->sendFile(source_sock, file_name); });
            t1.detach();
        }
    }
    else
    {
        return;
    }
}

void Responser::sendFile(int source_sock, string file_name)
{
    std::ifstream inFile(file_name, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return;
    }

    // 讀取檔案並發送給source_sock
    char buffer[256 * 1024];
    while (inFile.read(buffer, sizeof(buffer)) || inFile.gcount() > 0)
    {
        ssize_t bytesSent = send(source_sock, buffer, inFile.gcount(), 0);
        if (bytesSent < 0)
        {
            std::cerr << "Failed to send file data to socket: " << source_sock << std::endl;
            break;
        }
    }
    inFile.close();
    std::cout << "File " << file_name << "success sent to socket " << source_sock << std::endl;
}

void Responser::receiveFile(ssize_t bytesRead, char *buf)
{
    // 創建本地文件
    std::ofstream outFile(file_name, std::ios::app | std::ios::binary);
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
        std::cout << "file chunk received" << std::endl;
    }

    if (offset >= file_size)
    {
        filetranfermode = false;

        std::cout << "File received and saved to " << file_name << std::endl;

        // 通知接收端是否接受檔案
        Json::Value response;
        response["type"] = "file_recv";
        response["file_name"] = file_name;
        response["file_size"] = file_size;
        response["from"] = file_src;
        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, response);
        write(file_dst, jsonString.c_str(), jsonString.size());
    }
    outFile.close();
}

void Responser::enqueueFileChunk(int clientSock, const std::string &chunk)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    fileChunksQueue.push({clientSock, chunk});
    queueCondVar.notify_one();
}

// 這個函式會運行在獨立的線程中，持續將 queue 中的區塊發送給目標客戶端
void Responser::processFileChunks()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondVar.wait(lock, [this]
                          { return !fileChunksQueue.empty(); });

        auto [clientSock, chunk] = fileChunksQueue.front();
        fileChunksQueue.pop();
        lock.unlock();

        // 這裡進行檔案區塊的發送
        sendFileChunkToClient(clientSock, chunk, "");
    }
}

void Responser::sendFileChunkToClient(int target_sock, const std::string filename, const std::string &chunk)
{
    Json::Value response;
    response["type"] = "file_chunk";
    response["file_name"] = filename;
    response["content"] = chunk;

    Json::StreamWriterBuilder writer;
    std::string jsonString = Json::writeString(writer, response);
    write(target_sock, jsonString.c_str(), jsonString.size()); // 發送區塊至目標客戶端
}

Responser::Responser(/* args */)
{
}

Responser::~Responser()
{
}