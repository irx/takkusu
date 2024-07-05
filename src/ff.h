typedef struct {
	size_t w, h, siz;
	float *d;
} Image;

Image * ff_load(const char *);
size_t snd_load(int16_t **, const char *);
