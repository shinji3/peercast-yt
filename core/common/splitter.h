#ifndef _SPLITTER_H
#define _SPLITTER_H

#include "stream.h"

// �f�[�^�X�g���[���𕡐��̃X�g���[���ɕ��򂷂�Bclose ���A���邢�̓f
// �R���X�g���N�g���ɏo�͐�̃X�g���[���� close �����B
struct StreamSplitter : public Stream
{
    StreamSplitter(const std::vector<Stream*>& streams = {})
        : Stream()
        , m_streams(streams) {}

    ~StreamSplitter()
    {
        close();
    }

    void addStream(Stream* stream)
    {
        m_streams.push_back(stream);
    }

    const std::vector<Stream*>&
    streams()
    {
        return m_streams;
    }

    int read(void *buf, int len) override
    {
        throw StreamException("Stream can`t read");
    }

    void write(const void *data, int len) override
    {
        for (auto& stream : m_streams)
            stream->write(data, len);
    }

    void close() override
    {
        for (auto& stream : m_streams)
            stream->close();
    }

    std::vector<Stream*> m_streams;
};

#endif
