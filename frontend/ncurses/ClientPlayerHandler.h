#ifndef CLIENTPLAYERHANDLER_H
#define CLIENTPLAYERHANDLER_H

#include <scx/Signal.hpp>
#include <scx/Function.hpp>
using namespace scx;

#include "Protocol.h"

#define SEND_PACKET(stream)  \
{\
    int payloadSize = (BufObj(NULL) stream).Offset();   \
    char* buf = fnGetPayloadBuffer(                     \
            Protocol::Op::Group::Player, payloadSize);  \
    BufObj(buf) stream;                                 \
}\
    fnSendOut()

class ClientPlayerHandler
{
    friend class Client;

public:
    ClientPlayerHandler()
    {
    }

    ~ClientPlayerHandler()
    {
    }

    void Handle(char* buf, int len)
    {
        using namespace Protocol;

        if (len < (int)sizeof(char))
            return;

        char op = Op::Player::None;

        BufObj bufObj(buf);
        bufObj >> op;
        switch (op) {
            case Op::Player::ItemProgress:
                break;

            default:
                break;
        }
    }

public:
    void Play()
    {
    }

    void Pause()
    {
    }

    void Resume()
    {
    }

    void Stop()
    {
    }

    void Next()
    {
    }

    void Previous()
    {
    }

    void VolumeUp()
    {
    }

    void VolumeDown()
    {
    }

private:
    Function<char* (char, int)> fnGetPayloadBuffer;
    Function<void (void)> fnSendOut;

    Signal<void ()> m_SigStatusChanged;
};

#undef SEND_PACKET
#endif