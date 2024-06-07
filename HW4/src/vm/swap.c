#include "vm/swap.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

struct block *swap_block;
struct lock swap_lock;

void swap_init(void)
{
  lock_init(&swap_lock);
  swap_bitmap = bitmap_create(PGSIZE);
  return;
}

void swap_in(size_t index, void *kaddr)
{
  if (!index)
  {
    NOT_REACHED();
    return;
  }

  index--;
  struct block *swap_block = block_get_role(BLOCK_SWAP);
  lock_acquire(&swap_lock);
  for (size_t block_offset = 0; block_offset < 8; block_offset++)
  {
    size_t sector = (index << 3) + block_offset;
    void *buffer = kaddr + (BLOCK_SECTOR_SIZE * block_offset);
    block_read(swap_block, sector, buffer);
  }
  bitmap_set_multiple(swap_bitmap, index, 1, false);
  lock_release(&swap_lock);
  return;
}

size_t swap_out(void *kaddr)
{
  swap_block = block_get_role(BLOCK_SWAP);
  lock_acquire(&swap_lock);
  size_t swap_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
  for (size_t block_offset = 0; block_offset < 8; block_offset++)
  {
    size_t sector = (swap_index << 3) + block_offset;
    void *buffer = kaddr + (BLOCK_SECTOR_SIZE * block_offset);
    block_write(swap_block, sector, buffer);
  }
  lock_release(&swap_lock);
  return swap_index + 1;
}

void swap_free(size_t index)
{
  if (!index)
    return;

  lock_acquire(&swap_lock);
  bitmap_set_multiple(swap_bitmap, index - 1, 1, false);
  lock_release(&swap_lock);
  return;
}
