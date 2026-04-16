#include <EpollPoller.h>
#include <Poller.h>
#include <PollPoller.h>
#include <stdlib.h>

/*

    我们想要实现工厂产生poller,但是有不能在基类Poller引入派生类EpollPoller/PollPoller等内容.
    从逻辑上,基类包含派生类不合理;从实现上,可能会造成循环引用.
    因此,我们实现一个额外的DefaultPoller文件,将工厂函数置于其中,
    这样包含派生类的文件,就不会造成循环引用.

*/

Poller *Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        return new PollPoller(loop);
    } else {
        return new EpollPoller(loop);
    }
}
