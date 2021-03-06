#include "sway_socket.hpp"

// using namespace swayipc::data;

namespace swayipc {

sway_socket::sway_socket()
    : m_socket_path(std::getenv("SWAYSOCK")), m_socket(AF_UNIX, SOCK_STREAM, 0) {}

bool sway_socket::connect(std::string socket_path) {
    m_socket_path = socket_path;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::copy(m_socket_path.begin(), m_socket_path.end(), addr.sun_path);

    return m_socket.connect((const sockaddr&)addr, sizeof(addr));
}

bool sway_socket::connect() {
    if (m_socket_path.empty()) {
        throw std::runtime_error("Sway socket path not found not found\n");
    }
    return connect(m_socket_path);
}

void sway_socket::close() {
    m_socket.close();
}

#define ANY 0xFFFFFFFF

void sway_socket::handle_events() {
    message_s message;

    std::unique_lock lock{m_read_mutex};
    message.header = peek_header();
    if (!is_event(message)) return;
    message = read(message_type(ANY));

    send_to_event_queue(std::move(message));
}

void sway_socket::send_to_event_queue(message_s&& msg) {
    Expects(is_event(msg));
    m_queues[msg.header.type].push(msg);
}

client sway_socket::get_client() {
    return client(this);
}

message_s sway_socket::request(message_type type, std::string payload) {
    write(type, payload);
    return read(type);
}

static size_t read_num = {};
message_s sway_socket::read(message_type type) {
    std::optional<size_t> res;
    message_s message;

    while (true) {
        std::unique_lock lock{m_read_mutex};
        res = m_socket.recv(&message.header, sizeof(message.header));
        if (!res) throw std::runtime_error("read error");

        message.payload.resize(message.header.length);

        res = m_socket.recv(message.payload.data(), message.header.length);
        if (!res) throw std::runtime_error("read error");

        read_num++;
        if (message.header.type != type && type != ANY)
            send_to_event_queue(std::move(message));
        else break;
    }

    return message;
}

message_header sway_socket::peek_header() {
    std::optional<size_t> res;
    message_header header;

    std::unique_lock lock{m_read_mutex};
    res = m_socket.recv(&header, sizeof(header), MSG_PEEK);
    if (!res) throw std::runtime_error("read error");

    return header;
}

void sway_socket::write(message_type type, std::string payload) {
    message_s message;
    message.header.type = type;
    message.header.length = payload.size();
    message.payload = std::move(payload);

    std::optional<size_t> res;

    std::unique_lock lock{m_write_mutex};

    res = m_socket.send((const char*)&message.header, sizeof(message.header));
    if (!res) throw std::runtime_error("send error");

    if (message.header.length != 0) {
        res = m_socket.send(message.payload.data(), message.header.length);
        if (!res) throw std::runtime_error("send error");
    }
}

}
