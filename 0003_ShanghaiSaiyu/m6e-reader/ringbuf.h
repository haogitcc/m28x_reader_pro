
struct ringbuf;

struct ringbuf* rng_buf_create(int buf_size, int elem_size);
void rng_buf_delete(struct ringbuf* rng);

void rng_buf_reset(struct ringbuf* rng);

int rng_buf_len(struct ringbuf* rng);

void rng_buf_write(struct ringbuf* rng, char *buf, int len);
int rng_buf_enter(struct ringbuf* rng, char *buf, int len);

int rng_buf_read(struct ringbuf* rng, char *buf, int size);
int rng_buf_send(struct ringbuf* rng, int fd);

