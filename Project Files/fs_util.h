//---METHOD INSTANTIATION(S)---
int get_free_inode();
int get_free_block();
int rand_string( char * str, size_t size );
int format_timeval( struct timeval * tv, char * buf, size_t sz );
char get_bit( char * array, int index );
void set_bit( char * array, int index, char value );