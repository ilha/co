#include "co/all.h"

#ifdef CO_SSL
DEF_string(ip, "127.0.0.1", "ip");
DEF_int32(port, 9988, "port");
DEF_string(key, "", "private key file");
DEF_string(ca, "", "certificate file");
DEF_int32(t, 0, "0: server & client, 1: server, 2: client");
DEC_bool(cout);

struct Header {
    int32 magic;
    int32 body_len;
};

void on_connection(SSL* s) {
    const char* msg = "hello client";
    Header header;
    fastream buf(128);
    int32 body_len;
    int r;

    int fd = ssl::get_fd(s);
    if (fd < 0) goto err;

    while (true) {
        r = ssl::recvn(s, &header, sizeof(header), 3000);
        if (r != sizeof(header)) {
            ELOG << "ssl server recvn error: " << ssl::strerror(s);
            goto err;
        }

        body_len = ntoh32(header.body_len);
        LOG << "server recv header, body_len: " << body_len;

        buf.clear();
        r = ssl::recvn(s, (char*)buf.data(), body_len, 3000);
        if (r != body_len) {
            ELOG << "ssl server recvn error: " << ssl::strerror(s);
            goto err;
        }

        buf.resize(body_len);
        LOG << "server recv: " << buf;

        header.magic = hton32(777);
        header.body_len = hton32((uint32)strlen(msg));
        buf.clear();
        buf.append(&header, sizeof(header));
        buf.append(msg);

        r = ssl::send(s, buf.data(), (int)buf.size(), 3000);
        if (r != (int)buf.size()) {
            ELOG << "ssl server send error: " << ssl::strerror(s);
            goto err;
        }
    }

  err:
    ssl::shutdown(s);
    ssl::free_ssl(s);
    if (fd >= 0) { co::close(fd, 1000); fd = -1; }
}

void client_fun() {
    ssl::Client c(FLG_ip.c_str(), FLG_port);
    if (!c.connect(3000)) {
        LOG << "ssl connect failed: " << ssl::strerror();
        return;
    }

    const char* msg = "hello server";
    Header header;
    fastream buf(128);
    int32 body_len;
    int r;

    while (true) {
        header.magic = hton32(777);
        header.body_len = hton32((uint32)strlen(msg));
        buf.clear();
        buf.append(&header, sizeof(header));
        buf.append(msg);

        int r = c.send(buf.data(), (int)buf.size(), 3000);
        if (r != (int)buf.size()) {
            ELOG << "ssl client send error: " << ssl::strerror(c.ssl());
            goto err;
        }

        r = c.recvn(&header, sizeof(header), 3000);
        if (r != sizeof(header)) {
            ELOG << "ssl client recvn error: " << ssl::strerror(c.ssl());
            goto err;
        }

        body_len = ntoh32(header.body_len);
        LOG << "client recv header, body_len: " << body_len;

        buf.clear();
        r = c.recvn((char*)buf.data(), body_len, 3000);
        if (r != body_len) {
            ELOG << "ssl client recvn error: " << ssl::strerror(c.ssl());
            goto err;
        }

        buf.resize(body_len);
        LOG << "client recv: " << buf << '\n';

        co::sleep(2000);
    }

  err:
    c.disconnect();
    return;
}

int main(int argc, char** argv) {
    flag::init(argc, argv);
    FLG_cout = true;
    log::init();

    ssl::Server serv;
    serv.on_connection(on_connection);

    if (FLG_t == 0) {
        serv.start(FLG_ip.c_str(), FLG_port, FLG_key.c_str(), FLG_ca.c_str());
        sleep::ms(32);
        go(client_fun);
    } else if (FLG_t == 1) {
        serv.start(FLG_ip.c_str(), FLG_port, FLG_key.c_str(), FLG_ca.c_str());
    } else {
        go(client_fun);
    }

    while (true) sleep::sec(1024);
}
#else
int main(int argc, char** argv) {
    COUT << "openssl required..";
    return 0;
}
#endif
