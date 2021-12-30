typedef struct {
	size_t w, h, siz;
	float *d;
} Image;

Image * ff_load(const char *);
