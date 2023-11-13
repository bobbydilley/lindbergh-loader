#define JVSBUFFER_SIZE 1024

typedef struct {
    int ctsCounter;
    int ready;
    int size;
    char buffer[JVSBUFFER_SIZE];
} JVSFrame;

int getDCD(int fd);
int getDSR(int fd);
int getCTS(int fd);

JVSFrame readJVSFrameFromThread();
int startJVSFrameThread(int * fd);
void * readJVSFrameThread(void * arg);

int openJVSSerial(char *jvsPath);
int initJVSSerial(int fd);
int readJVSFrame(int fd, unsigned char *buffer);
