#define CHUNKWIDTH 16
#define CHUNKAREA CHUNKWIDTH * CHUNKWIDTH
#define SECTIONS 16
#define SECHEIGHT 16
#define SECSIZE SECHEIGHT * CHUNKAREA
#define CHUNKHEIGHT SECHEIGHT * SECTIONS
#define CHUNKSIZE CHUNKHEIGHT * CHUNKAREA


unsigned char* get_chunk_heightmap(nbt_node* chunk);
unsigned char* get_chunk_blocks(nbt_node* chunk);
