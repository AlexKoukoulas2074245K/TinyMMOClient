///------------------------------------------------------------------------------------------------
///  WindowsUtils.cpp
///  Predators
///
///  Created by Alex Koukoulas on 20/01/2024.
///-----------------------------------------------------------------------------------------------

#include <engine/utils/ThreadSafeQueue.h>
#include <platform_utilities/WindowsUtils.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

// Link with ws2_32.lib
#pragma comment(lib, "ws2_32.lib")
///-----------------------------------------------------------------------------------------------

namespace windows_utils
{

///-----------------------------------------------------------------------------------------------

bool IsConnectedToTheInternet()
{
    return false; // InternetCheckConnection("http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
}

///-----------------------------------------------------------------------------------------------

std::string GetPersistentDataDirectoryPath()
{
#define _CRT_SECURE_NO_WARNINGS
    auto appDataLocation = getenv("APPDATA");
    return std::string(appDataLocation) + "/RealmofBeasts/";
}

///-----------------------------------------------------------------------------------------------

class MessageSender
{
public:
    MessageSender() : mCanSendNetworkMessage(true) { Start(); }

    void Start()
    {
        mThread = std::thread([&]
        {
            while (true)
            {
                auto messageJobToSend = mMessageQueueToSend.dequeue();                    
                //mCanSendNetworkMessage = false;
                const auto startTime = std::chrono::system_clock::now();

                networking::ServerResponseData responseData = {};
                SOCKET clientSocket = 0;

                auto responseErrorLambda = [&](const std::string&& errorMessage)
                {
                    mCanSendNetworkMessage = true;
                    closesocket(clientSocket);
                    WSACleanup();

                    responseData.mError = std::move(errorMessage);
                    messageJobToSend.second(responseData);
                };

                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
                {
                    responseErrorLambda("WSAStartup failed.");
                    return;
                }

                // Create socket
                clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (clientSocket == INVALID_SOCKET)
                {
                    responseErrorLambda("Error: Socket creation failed");
                    return;
                }

                // Specify server address
                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(8070); // Use the same port as the server        
                inet_pton(AF_INET, "178.16.131.241", &serverAddr.sin_addr); // Use the server's IP address
                //inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // Use the server's IP address

                // Connect to server
                if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
                    responseErrorLambda("Error: Connection failed");
                    return;
                }
                    
                if (send(clientSocket, messageJobToSend.first.c_str(), messageJobToSend.first.size(), 0) == SOCKET_ERROR)
                {
                    responseErrorLambda("Error: Send Failed");
                    return;
                }

                // Send null character to indicate end of message
                char nullTerminator = '\0';
                if (send(clientSocket, &nullTerminator, sizeof(nullTerminator), 0) == SOCKET_ERROR)
                {
                    responseErrorLambda("Error: Send Failed");
                    return;
                }

                while (true)
                {
                    char buffer[4096];
                    auto bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesReceived == -1)
                    {
                        responseErrorLambda("Error: recv() Message too large!");
                        break;
                    }
                    else if (bytesReceived == 0)
                    {
                        break;
                    }
                    else
                    {
                        responseData.mResponse.append(buffer, bytesReceived);
                        if (responseData.mResponse.find('\0') != std::string::npos)
                        {
                            // Null character found, indicating end of JSON message
                            break;
                        }
                    }
                }

                if (!responseData.mResponse.empty())
                {
                    mCanSendNetworkMessage = true;
                    const auto endTime = std::chrono::system_clock::now();
                    responseData.mPingMillis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                    messageJobToSend.second(responseData);
                }

                // Close socket
                closesocket(clientSocket);
                WSACleanup();
                mCanSendNetworkMessage = true;
            }
        });
        mThread.detach();
    }

    void SendMessage(const nlohmann::json& networkMessage, const networking::MessageType messageType, const bool highPriority, std::function<void(const networking::ServerResponseData&)> serverResponseCallback)
    {
        if (mCanSendNetworkMessage || highPriority)
        {
            auto finalNetworkMessageJson = networkMessage;
            networking::PopulateMessageHeader(finalNetworkMessageJson, messageType);
            mMessageQueueToSend.enqueue(std::make_pair(finalNetworkMessageJson.dump(), serverResponseCallback));
        }
    }

private:
    std::thread mThread;
    std::atomic<bool> mCanSendNetworkMessage;
    ThreadSafeQueue<std::pair<std::string, std::function<void(const networking::ServerResponseData&)>>> mMessageQueueToSend;
};


void SendNetworkMessage(const nlohmann::json& networkMessage, const networking::MessageType messageType, const bool highPriority, std::function<void(const networking::ServerResponseData&)> serverResponseCallback)
{
    static MessageSender messageSender;
    messageSender.SendMessage(networkMessage, messageType, highPriority, serverResponseCallback);
}

///----------------------------------------------------------------------------------------------v-

}
