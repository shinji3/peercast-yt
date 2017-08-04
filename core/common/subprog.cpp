#include "subprog.h"
#include <stdexcept>

// �v���O�����̏o�͂�ǂݏo���X�g���[���B
Stream& Subprogram::inputStream()
{
    if (!m_receiveData)
        throw std::runtime_error("no input stream");
    return m_inputStream;
}

Stream& Subprogram::outputStream()
{
    if (!m_feedData)
        throw std::runtime_error("no output stream");
    return m_outputStream;
}

// �v���Z�XID�B
int Subprogram::pid()
{
    return m_pid;
}
