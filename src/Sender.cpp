#include "Sender.hpp"
bool Sender::startServerAccept()
{
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock < 0)
    {
        std::cerr << "ERR: cannot open listen socket." << std::endl;
        return false;
    }
    
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in srv_addr;

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if(bind(listen_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
    {
        std::cerr << "ERR: bind err." << std::endl;
        close(listen_sock);
        return false;
    }

    listen(listen_sock,1);
    std::cout << "waiting on port " << port << std::endl;

    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    conn_sock = accept(listen_sock, (struct sockaddr*)&cli_addr, &cli_len);

    close(listen_sock);
    listen_sock = -1;

    if(conn_sock < 0)
    {
        std::cerr << "ERR: accept failed" << std::endl;
        return false;
    }

    std::cout << "client connected" << std::endl;

    return true;
}
bool Sender::sendData(void* data, size_t size_data)
{
    if(conn_sock < 0)
        return false;
    //ssize_t muze ulozit i -1 [more you know :)]
    //ssize_t n_bytes = write(conn_sock, data, size_data);
    ssize_t n_bytes = send(conn_sock, data, size_data, MSG_NOSIGNAL);

    if(n_bytes != size_data)
    {
        std::cout << "client disconnected" << std::endl;
        close(conn_sock);
        return false;
    }
    return true;
}