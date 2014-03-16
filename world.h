unsigned char* render_world_blockmap(const char* worlddir, const colour* colours,
		const char night);
unsigned char* render_world_iso_blockmap(const char* worlddir, const colour* colours,
		const char night);

void save_world_blockmap(const char* worlddir, const char* imagefile, const colour* colours,
		const char night, const char isometric);
