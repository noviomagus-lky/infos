/*
 * Buddy Page Allocation Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (2)
 */

/*
 * STUDENT NUMBER: s1891130
 */
#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	17

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:
	/**
	 * Returns the number of pages that comprise a 'block', in a given order.
	 * @param order The order to base the calculation off of.
	 * @return Returns the number of pages in a block, in the order.
	 */
	static inline constexpr uint64_t pages_per_block(int order)
	{
		/* The number of pages per block in a given order is simply 1, shifted left by the order number.
		 * For example, in order-2, there are (1 << 2) == 4 pages in each block.
		 */
		return (1 << order);
	}
	
	/**
	 * Returns TRUE if the supplied page descriptor is correctly aligned for the 
	 * given order.  Returns FALSE otherwise.
	 * @param pgd The page descriptor to test alignment for.
	 * @param order The order to use for calculations.
	 */
	static inline bool is_correct_alignment_for_order(const PageDescriptor *pgd, int order)
	{
		// Calculate the page-frame-number for the page descriptor, and return TRUE if
		// it divides evenly into the number pages in a block of the given order.
		return (sys.mm().pgalloc().pgd_to_pfn(pgd) % pages_per_block(order)) == 0;
	}
	
	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		// (1) Make sure 'order' is within range
		if (order >= MAX_ORDER) {
			return NULL;
		}

		// (2) Check to make sure that PGD is correctly aligned in the order
		if (!is_correct_alignment_for_order(pgd, order)) {
			return NULL;
		}
				
		// (3) Calculate the page-frame-number of the buddy of this page.
		// * If the PFN is aligned to the next order, then the buddy is the next block in THIS order.
		// * If it's not aligned, then the buddy must be the previous block in THIS order.
		uint64_t buddy_pfn = is_correct_alignment_for_order(pgd, order + 1) ?
			sys.mm().pgalloc().pgd_to_pfn(pgd) + pages_per_block(order) : 
			sys.mm().pgalloc().pgd_to_pfn(pgd) - pages_per_block(order);
		
		// (4) Return the page descriptor associated with the buddy page-frame-number.
		return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}
	
	/**
	 * Inserts a block into the free list of the given order.  The block is inserted in ascending order.
	 * @param pgd The page descriptor of the block to insert.
	 * @param order The order in which to insert the block.
	 * @return Returns the slot (i.e. a pointer to the pointer that points to the block) that the block
	 * was inserted into.
	 */
	PageDescriptor **insert_block(PageDescriptor *pgd, int order)
	{
		// Starting from the _free_area array, find the slot in which the page descriptor
		// should be inserted.
		PageDescriptor **slot = &_free_areas[order];
		
		// Iterate whilst there is a slot, and whilst the page descriptor pointer is numerically
		// greater than what the slot is pointing to.
		while (*slot && pgd > *slot) {
			slot = &(*slot)->next_free;
		}
		
		// Insert the page descriptor into the linked list.
		pgd->next_free = *slot;
		*slot = pgd;
		
		// Return the insert point (i.e. slot)
		return slot;
	}
	
	/**
	 * Removes a block from the free list of the given order.  The block MUST be present in the free-list, otherwise
	 * the system will panic.
	 * @param pgd The page descriptor of the block to remove.
	 * @param order The order in which to remove the block from.
	 */
	PageDescriptor *remove_block(PageDescriptor *pgd, int order)
	{
		// Starting from the _free_area array, iterate until the block has been located in the linked-list.
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}

		// Make sure the block actually exists.  Panic the system if it does not.
		assert(*slot == pgd);
		
		// Remove the block from the free list.
		*slot = pgd->next_free;
		pgd->next_free = NULL;
		
		return pgd;
	}
	
	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
		// Make sure there is an incoming pointer.
		assert(*block_pointer);
		
		// Make sure the block_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));
		
		// TODO: Implement this function
		
		// (1) Make sure the block can be further splitted.
		assert(source_order > 0);

		// (2) Remove the old block.
		PageDescriptor *temp = *block_pointer;
		remove_block(temp, source_order);

		// (3) Insert the two child blocks.
		insert_block(buddy_of(temp, source_order - 1), source_order - 1);	
		
		insert_block(temp, source_order - 1);

		// (4) Return the pointer to the left-hand-side of the new block.
		return temp;
	}
	
	/**
	 * Takes a block in the given source order, and merges it (and it's buddy) into the next order.
	 * This function assumes both the source block and the buddy block are in the free list for the
	 * source order.  If they aren't this function will panic the system.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
	
		assert(*block_pointer);
		
		// Make sure the area_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		// TODO: Implement this function

		// (1) Make sure the block can be further merged.
		assert(source_order < MAX_ORDER - 1);

		// (2) Remove the old block and its buddy.
		PageDescriptor *buddy = buddy_of(*block_pointer, source_order);
		PageDescriptor *pgd = *block_pointer; 
		remove_block(pgd, source_order);
		remove_block(buddy, source_order);

		// (3) Insert the merged block of two and return the new slot pointing to the merged block.	
		PageDescriptor *temp;
		if (is_correct_alignment_for_order(pgd, source_order + 1))
		return insert_block(pgd, source_order + 1);
		else
		return insert_block(buddy, source_order + 1);
	}
	
public:
	/**
	 * Constructs a new instance of the Buddy Page Allocator.
	 */
	BuddyPageAllocator() {
		// Iterate over each free area, and clear it.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			_free_areas[i] = NULL;
		}
	}
	
	/**
	 * Allocates 2^order number of contiguous pages.
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *alloc_pages(int order) override
	{	
		PageDescriptor **slot = &_free_areas[order];

		// (1) Check if we already have a block at this order.
		if (*slot != NULL) {
			return remove_block(*slot, order);
		}
	
		// (2) Search for an available larger block.
		int i;
		for (i = order; i < MAX_ORDER; i++) {
			slot = &_free_areas[i];

			//Found a larger block, so break.
			if (*slot != NULL)
			break;	
		}
		
		// (3) No available block exists, allocation failed.
		if (i == MAX_ORDER)
			return NULL;
	
		// (4) If a larger block found, keep splitting it until the left-hand-side of new block fits the size.
		for (; i > order; i--) {
			split_block(slot, i);
			slot = &_free_areas[i - 1];
			
		}
	
		return remove_block(*slot, order);
	}
	
	/**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
	void free_pages(PageDescriptor *pgd, int order) override
	{
		// Make sure that the incoming page descriptor is correctly aligned.
		// for the order on which it is being freed, for example, it is
		// illegal to free page 1 in order-1.
		assert(is_correct_alignment_for_order(pgd, order));
		
		// (1) Check if the reference is valid, i.e.the block for the order has been allocated.
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}
		assert(slot != NULL);

		// (2) Insert the freed block.
		slot = insert_block(pgd, order);

		// (3) Iteritively check if the buddy of the freed block is also free so that they can be merged.	
		for (int i = order; i < MAX_ORDER - 1; i++) {
			//Search for buddy in the free list.
			PageDescriptor **buddy_slot = &_free_areas[i];
			while (*buddy_slot && *buddy_slot != buddy_of(*slot, i)) {
				buddy_slot = &(*buddy_slot) ->next_free;
			}

			//If the buddy has been allocated, break.
			if (*buddy_slot == NULL)
				break;
			else
				slot = merge_block(slot, i);
		}
	}
	
	/**
	 * Reserves a specific page, so that it cannot be allocated.
	 * @param pgd The page descriptor of the page to reserve.
	 * @return Returns TRUE if the reservation was successful, FALSE otherwise.
	 */
	bool reserve_page(PageDescriptor *pgd)
	{
		// (1) Search for the block that contains the given page.
		int i = MAX_ORDER - 1;
		PageDescriptor **slot = &_free_areas[i];
		uint64_t pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
		uint64_t slot_pfn = sys.mm().pgalloc().pgd_to_pfn(*slot);
		
		//For each block in each order, search for the target block.
		for (; i >= 0 ; i--) {
			slot = &_free_areas[i];
			slot_pfn = sys.mm().pgalloc().pgd_to_pfn(*slot);
			int nr_page_block = pages_per_block(i);

			while (*slot && (slot_pfn > pfn || slot_pfn + nr_page_block - 1< pfn)) {
				
				//Refresh the PFN value of slot whenever slot changes.
				slot = &(*slot)->next_free;
				slot_pfn = sys.mm().pgalloc().pgd_to_pfn(*slot);
			}

			//Break when we found the target block.
			if (*slot != NULL)
				break;
		}

		// (2) Return false if the given page is allocated.
		if (*slot == NULL) {
			return false;
		}
		
		// (3) If the given page is in the first order, we just remove it and return.
		if (i == 0) { 
			remove_block(*slot, 0);
			return true;
		}
		
		// (4) Else, we keep splitting the block until we get the single page we need.
		PageDescriptor *temp = *slot;
		for (; i > 0; i--) {
			temp = split_block(&temp, i);
			uint64_t temp_pfn = sys.mm().pgalloc().pgd_to_pfn(temp);

			if (temp_pfn + pages_per_block(i - 1) - 1 < pfn) {
				temp = buddy_of(temp, i - 1);
			}
		}
	
		remove_block(temp, 0);
		return true;
	}
	
	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
		mm_log.messagef(LogLevel::DEBUG, "Buddy Allocator Initialising pd=%p, nr=0x%lx", page_descriptors, nr_page_descriptors);
		
		// TODO: Initialise the free area linked list(free list) for the maximum order.
		// to initialise the allocation algorithm.
		
		// First check if the free list is vacant.
		for (int i = 0;i < MAX_ORDER; i++) {
			if (_free_areas[i] != NULL)
				return false;
		}

		// Check if the value of MAX_ORDER is a reasonable value.
		int max_pages_per_block = pages_per_block(MAX_ORDER - 1);
		assert(nr_page_descriptors >= max_pages_per_block);

		// Initially, seperate all pages of memory into two parts, because the number of pages may not just fit in
		// the MAX_ORDER, thus leaves rest part of pages to be initialised.
		int main_part = nr_page_descriptors/max_pages_per_block;
		int rest_part = nr_page_descriptors%max_pages_per_block;

		// Assign main part of pages to the free list for the maximum order.
		PageDescriptor **slot = &_free_areas[MAX_ORDER - 1];
		*slot = page_descriptors;
		main_part -= 1;

		while (main_part > 0) {	  
		 	(*slot)->next_free = *slot + max_pages_per_block;
			slot = &(*slot)->next_free;
			main_part -= 1;
		}

		// Assign the rest part of pages. 
		PageDescriptor *rest_pgd = *slot + max_pages_per_block;
		for (int i = MAX_ORDER - 2; i >= 0; i--) {
			if (rest_part >= pages_per_block(i)) {
				_free_areas[i] = rest_pgd;
				rest_pgd += pages_per_block(i);
				rest_part -= pages_per_block(i);
			}
		}

		return true;		
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }
	
	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");
		
		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);
						
			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}
			
			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}
	
private:
	PageDescriptor *_free_areas[MAX_ORDER];

};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);
