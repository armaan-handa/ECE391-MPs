#define PAGE_TABLE_SIZE 1024
#define FOUR_KI_B 4096
#define FOUR_MI_B 4194304
#define VIDEO_PAGE 0xB8
#define VIDEO_ADDR 0xB8000

extern void allow_paging(unsigned int page_directory);
extern void init_pages();
extern void page_on_4mb (void * phys_addr, void * virt_addr);
extern void page_off_4mb (void * virt_addr);
extern void page_on_4kb (void * phys_addr, void * virt_addr);
extern void video_page_remap (void * phys_addr, void * virt_addr);
