#include <iostream>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>

#define EVENT_SIZE      (sizeof(struct inotify_event))
#define BUF_LEN         (1024 * (EVENT_SIZE + 16))

std::string get_timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = now.time_since_epoch();
    auto diff = duration_cast<milliseconds>(ms).count();
    auto const msecs = diff % 1000;

    std::time_t time_tt = system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&time_tt, &tm);

    char timestamp[50] = { '\0' };

        sprintf(timestamp,
                "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
                tm.tm_year + 1900,
                tm.tm_mon + 1,
                tm.tm_mday,
                tm.tm_hour,
                tm.tm_min,
                tm.tm_sec,
                msecs);

    return timestamp;
}

int main(int argc, char * argv[]) {
    // Check if the command line arguments are correctly formatted
    if(argc != 2 || argv[1] == "")
    {
        std::cout << "Incorrect formatting" << std::endl;
        std::cout << "Please enter a format like {./te path}"<< std::endl;
        exit(EXIT_FAILURE);
    }

    //check path
    if(access(argv[1], F_OK))
    {
        perror("path not exit\n");
        exit(EXIT_FAILURE);
    }

    int fd, wd;
    char buffer[BUF_LEN];

    // Initialize inotify
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // Add the watch for the specified directory
    wd = inotify_add_watch(fd, argv[1], IN_ALL_EVENTS);
    if (wd < 0) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    std::cout << "Monitoring directory " << argv[1] << std::endl;

    // Main loop to monitor directory changes
    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            // Handle different types of events
            if (event->mask & IN_CREATE) {
                std::cout << "[" << get_timestamp() << "]" << " File "<< "[" << event->name << "]" << " created" << std::endl;
            } else if (event->mask & IN_DELETE) {
                std::cout << "[" << get_timestamp() << "]" << " File "<< "[" << event->name << "]" << " deleted" << std::endl;
            }
            i += EVENT_SIZE + event->len;
        }
    }

    // Remove the watch and close the inotify file descriptor
    if (inotify_rm_watch(fd, wd) < 0) {
        perror("inotify_rm_watch");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return 0;
}
