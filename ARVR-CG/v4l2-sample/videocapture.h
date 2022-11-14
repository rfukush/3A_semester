
typedef struct _buffer {
    void *start;
    size_t length;
} buffer;

extern char *dev_name;
extern int fd, width, height;
extern buffer *buffers;
extern unsigned int n_buffers;

void open_device(void);
void close_device(void);
void convert_rgb(void);
void process_image(const void *p);
/*
void write_ppm(void);
void write_ppm_file(char *filename);
void mainloop(void);
*/
static void usage(FILE * fp, int argc, char **argv);


