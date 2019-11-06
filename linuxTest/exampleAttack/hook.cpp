
#include <stdio.h>

#include <chickenHook/hooking.h>
#include <chickenHook/logging.h>

extern "C" void _Unwind_Resume() {

}

void logCallback(const std::string logtext) {

    printf("%s\n", logtext.c_str());

}

template<class Elem, class Traits>
static inline void
hex_dump(const void *aData, std::size_t aLength, std::basic_ostream<Elem, Traits> &aStream,
         std::size_t aWidth = 16) {
    const char *const start = static_cast<const char *>(aData);
    const char *const end = start + aLength;
    const char *line = start;
    while (line != end) {
        aStream.width(4);
        aStream.fill('0');
        aStream << std::hex << line - start << " : ";
        std::size_t lineLength = std::min(aWidth, static_cast<std::size_t>(end - line));
        for (std::size_t pass = 1; pass <= 2; ++pass) {
            for (const char *next = line; next != end && next != line + aWidth; ++next) {
                char ch = *next;
                switch (pass) {
                    case 1:
                        aStream << (ch < 32 ? '.' : ch);
                        break;
                    case 2:
                        if (next != line)
                            aStream << " ";
                        aStream.width(2);
                        aStream.fill('0');
                        aStream << std::hex << std::uppercase
                                << static_cast<int>(static_cast<unsigned char>(ch));
                        break;
                }
            }
            if (pass == 1 && lineLength != aWidth)
                aStream << std::string(aWidth - lineLength, ' ');
            aStream << " ";
        }
        aStream << std::endl;
        line = line + lineLength;
    }
}

ssize_t my_read(int __fd, void *__buf, size_t __count) {
    printf("read called [-] %d", __fd);
    int res = -1;
    ChickenHook::Trampoline trampoline;
    if (ChickenHook::Hooking::getInstance().getTrampolineByAddr((void *) &read, trampoline)) {
        printf("hooked function call original function");
        // printLines(hexdump(static_cast<const uint8_t *>(__buf), __count, "read"));
        trampoline.copyOriginal();
        res = read(__fd, __buf, __count);
        hex_dump(__buf, __count, std::cout);
        trampoline.reinstall();
        return res;
    } else {
        printf("hooked function cannot call original function");
    }
    return res;
}

char *my_strcpy(char *__dst, const char *__src) {
    printf("strcpy called [-] ");
    hex_dump(__src, strlen(__src), std::cout);
    char *res = nullptr;
    ChickenHook::Trampoline trampoline;
    if (ChickenHook::Hooking::getInstance().getTrampolineByAddr((void *) &strcpy, trampoline)) {
        printf("hooked function call original function");
        // printLines(hexdump(static_cast<const uint8_t *>(__buf), __count, "read"));
        trampoline.copyOriginal();
        res = strcpy(__dst, __src);
        trampoline.reinstall();
        return res;
    } else {
        printf("hooked function cannot call original function");
    }
    return res;
}

pthread_t hooksInstallThread;

void *installHooks(void *) {
    printf("Started install hooks\n");
    usleep(5 * 1000 * 1000);
    ChickenHook::Hooking::getInstance().setLoggingCallback(&logCallback);
    ChickenHook::Hooking::getInstance().hook((void *) &read, (void *) &my_read);
    ChickenHook::Hooking::getInstance().hook((void *) &strcpy, (void *) &my_strcpy);
    printf("Install hooks finished\n");
    return nullptr;
}

void __attribute__  ((constructor (102))) testInit() {
    printf("You are hacked\n");
    if (pthread_create(&hooksInstallThread, NULL, &installHooks, nullptr)) {
        printf("Error creating hooking thread!");
    }
}