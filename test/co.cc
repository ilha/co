#include "co/co.h"
#include "co/log.h"
#include "co/time.h"

co::Event ev;
co::Mutex mtx;
co::Pool pool;

int v = 0;
int n = 0;

void f1() {
    ev.wait();
    LOG << "f1()";
    {
        co::MutexGuard g(mtx);
        ++v;
    }

    int* p = (int*) pool.pop();
    if (!p) p = new int(atomic_inc(&n));
    pool.push(p);
}

void f2() {
    bool r = ev.wait(50);
    LOG << "f2() r: " << r;
}

void f3() {
    LOG << "f3()";
    ev.signal();
}

int main(int argc, char** argv) {
    flag::init(argc, argv);
    log::init();
    FLG_cout = true;

    for (int i = 0; i < 8; ++i) go(f1);
    go(f2);

    sleep::ms(100);
    go(f3);

    sleep::ms(200);

    LOG << "v: " << v;
    LOG << "n: " << n;

    return 0;
}
