#ifdef _UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "subprog.h"
#include "env.h"

Subprogram::Subprogram(const std::string& name, Environment& env)
    : m_name(name)
    , m_pid(-1)
    , m_env(env)
{
}

Subprogram::~Subprogram()
{
}

// �v���O�����̎��s���J�n�B
#ifdef WIN32
bool Subprogram::start()
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE stdoutRead;
    HANDLE stdoutWrite;

    CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0);
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION procInfo;
    STARTUPINFO startupInfo;
    bool success;

    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.hStdError = stdoutWrite;
    startupInfo.hStdOutput = stdoutWrite;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    // �W�����͂̃n���h���w�肵�Ȃ��Ă����̂��ȁH

    success = CreateProcess(NULL,
                            (char*) ("ruby " + m_name).c_str(),
                            NULL,
                            NULL,
                            TRUE,
                            0,
                            (PVOID) m_env.windowsEnvironmentBlock().c_str(),
                            NULL,
                            &startupInfo,
                            &procInfo);

    m_processHandle = procInfo.hProcess;

    int fd = _open_osfhandle((intptr_t) stdoutRead, _O_RDONLY);
    m_inputStream.openReadOnly(fd);

    CloseHandle(stdoutWrite);
    CloseHandle(procInfo.hThread);
}
#endif
#ifdef _UNIX
bool Subprogram::start()
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        char buf[1024];
        strerror_r(errno, buf, sizeof(buf));
        LOG_ERROR("pipe: %s", buf);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // �q�v���Z�X�B

        // �p�C�v�̓ǂݏo���������B
        close(pipefd[0]);

        // �������ݑ���W���o�͂Ƃ��ĕ�������B
        close(1);
        dup(pipefd[1]);

        close(pipefd[1]);

        int r = execle(m_name.c_str(),
                       m_name.c_str(), NULL,
                       m_env.env());
        if (r == -1)
        {
            // char buf[1024];
            // strerror_r(errno, buf, sizeof(buf));

            char *buf = strerror(errno);
            LOG_ERROR("execle: %s: %s", m_name.c_str(), buf);
            _exit(1);
        }
        /* NOT REACHED */
    }else
    {
        // �e�v���Z�X�B

        m_pid = pid;

        // �������ݑ������B
        close(pipefd[1]);

        m_inputStream.openReadOnly(pipefd[0]);
    }
    return true;
}
#endif

// �v���O�����ɂ��I���X�e�[�^�X���Ԃ��ꂽ�ꍇ�� true ��Ԃ��A�X�e�[
// �^�X�� *status �ɃZ�b�g�����B
#ifdef WIN32
bool Subprogram::wait(int* status)
{
    *status = 0;
    return true;
}
#endif
#ifdef _UNIX
bool Subprogram::wait(int* status)
{
    int r;

    waitpid(m_pid, &r, 0);
    if (WIFEXITED(r))
    {
        *status = WEXITSTATUS(r);
        return true;
    }
    else
    {
        return false;
    }
}
#endif

// �v���O�����̏o�͂�ǂݏo���X�g���[���B
Stream& Subprogram::inputStream()
{
    return m_inputStream;
}

// �v���Z�XID�B
int Subprogram::pid()
{
    return m_pid;
}