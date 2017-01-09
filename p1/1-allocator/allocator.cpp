#include "allocator.h"
#include <list>
#include <iostream>
#include <iterator>
#include <algorithm>
using namespace std;

void * Pointer::get() const 
{
	if (unit == Pointer::allocator->used_list.end()) return nullptr;
    return (char*)Pointer::allocator->base + unit->offset;
}

Pointer :: Pointer ()
{
    unit = allocator->used_list.end();
}

void MemoryUnit::Expansion (size_t add_size) {
        size += add_size; 
}

//return offset new unit
size_t MemoryUnit::Decrease (size_t substr_size) {
    size -= substr_size;
    return offset + size;
}

Allocator * Pointer::allocator;

Allocator::Allocator(void *_base, size_t _size) {
    base = _base;
    size = _size;
    MemoryUnit first_unit(0, size);
    free_list.push_back(first_unit);
    used_memory = 0;
    Pointer::allocator = this; 
}


Pointer Allocator::alloc(size_t N) {

	if (N == 0) {
        return Pointer(used_list.end());
	}
  
    for (auto iter = free_list.rbegin(); iter != free_list.rend(); ++iter) {
      	if (N <= iter->size) {
            size_t new_offset = iter->Decrease(N);
            Pointer ptr(used_list.end());

            MemoryUnit new_unit(new_offset, N);
            
            int succes_alloc = 0;
            if (new_offset < used_list.begin()->offset) {
                used_list.insert(used_list.begin(), new_unit);
                ptr.unit = used_list.begin();
                ++succes_alloc;
            }

            if (new_offset > used_list.rbegin()->offset) {
                used_list.push_back(new_unit);
                ptr.unit = --used_list.end();
                ++succes_alloc;
            }

            if (!succes_alloc) {
                std::list<MemoryUnit>::iterator next_iter;
                for (auto iter = used_list.begin(); iter != used_list.end(); ++iter)
                {
                    next_iter = iter;
                    ++next_iter;

                    if ((new_offset < next_iter->offset) && (new_offset > iter->offset)){
                        used_list.insert(next_iter, new_unit);
                        --next_iter;
                        ptr.unit = next_iter;
                        break;
                    }
                }
            }

            if (iter->size == 0) {
            	free_list.erase((++iter).base());
            }
            
            return ptr;
            break;
     	}
    }
     
    throw AllocError(AllocErrorType::NoMemory, "no memory :(");
}

void Allocator::free(Pointer &p) 
{

	int succes_free = 0;
	if (p.unit->size == -1) throw AllocError(AllocErrorType::InvalidFree, "Invalid free :(");

	if (p.unit->offset < (free_list.begin())->offset) {
		++succes_free;
		//insert in begin
		if (p.unit->offset + p.unit->size == (free_list.begin())->offset) {
            (free_list.begin())->Expansion(p.unit->size);
            (free_list.begin())->offset = p.unit->offset;
		} else {
			free_list.emplace_front(p.unit->offset, p.unit->size);
			
		}
	}

    if (p.unit->offset > (free_list.rbegin())->offset) {
        ++succes_free;
        //insert in end
        if ((free_list.back()).offset + (free_list.back()).size == p.unit->offset) {
            (free_list.back()).Expansion(p.unit->size);
        } else {
            free_list.emplace_back(p.unit->offset, p.unit->size);
        }
    }
    
    if (!succes_free) {
    	std::list<MemoryUnit>::iterator next_iter;
		for (auto iter = free_list.begin(); iter != free_list.end(); ++iter) {
			//insert unit between 2 unit, check union
			next_iter = iter;
            ++next_iter;
			if ((iter->offset < p.unit->offset) && (next_iter->offset > p.unit->offset)) {
				++succes_free;
				//union 3 unit
	        	if ((iter->offset + iter->size == p.unit->offset) && 
	        		(p.unit->offset + p.unit->size == next_iter->offset)) {
	        		iter->Expansion(p.unit->size);
	            	iter->Expansion(next_iter->size);
	            	free_list.erase(next_iter);
	            	break;
	        	}

	        	//union 1 and 2 uint
	        	if  ((iter->offset + iter->size == p.unit->offset) && 
	        		(p.unit->offset + p.unit->size < next_iter->offset)) {
                	iter->Expansion(p.unit->size);
	            	break;
	        	}

	        	//union 2 and 3 unit
	        	if  ((iter->offset + iter->size < p.unit->offset) && 
	        		(p.unit->offset + p.unit->size == next_iter->offset)) {
                	next_iter->Expansion(p.unit->size);
                	next_iter->offset = p.unit->offset;
	            	break;
	        	}
	        	//just insert
	        	free_list.emplace(next_iter, p.unit->offset, p.unit->size);
	        	break;
			}
		}
	}


    
    for (auto iter = used_list.begin(); iter != used_list.end(); ++iter){
        if ((*iter).offset == p.unit->offset) {
             used_list.erase(iter);
             break;
        }
    }
    
	p.unit = used_list.end();
	
}

void Allocator::realloc(Pointer &p, size_t N)
{
	if (p.unit == used_list.end()) {
		p = (*this).alloc(N);
		return;
	}
	if (N <= p.unit->size) {
        Pointer for_free(used_list.end());
        MemoryUnit new_unit(p.unit->offset + N, p.unit->size - N);
        used_list.insert(++p.unit, new_unit);
		for_free.unit = ++p.unit;

        p.unit->size = N;

        free(for_free);
	} else {
		char * for_copy = (char *)p.get();
		free(p);
        Pointer new_p = (*this).alloc(N);

        for (size_t i  = 0; i < N; ++i) {
        	*(((char*)new_p.get()) + i)= *(for_copy + i);
        }

        p.unit = new_p.unit;
	}
}

void Allocator::defrag() 
{
    char * end_memory = (char *)base + (size - 1);
    for (auto iter = used_list.rbegin(); iter != used_list.rend(); ++iter)
    {
        for (int s = iter->size - 1; s >= 0; --s)
        {
            *end_memory = *((char *)base + iter->offset + s);
            end_memory = end_memory - 1;
        }
        iter->offset = end_memory + 1 - (char *)base;
    }

   
    
    auto new_free_unit = free_list.begin();
    new_free_unit->size = end_memory + 1 - (char *) base;
    new_free_unit->offset = 0;
    ++new_free_unit;

    while (new_free_unit != free_list.end()) {
        new_free_unit = free_list.erase(new_free_unit);
    }
}

void Allocator::dump() {

    cout << "_________________FREE_UNIT______________"<< endl;
    int i = 0;
    for (auto iter = free_list.begin(); iter != free_list.end(); ++iter) {
        cout << "unit "<< i<< ":" << "offset: " << iter->offset << " size: " << iter->size<< endl;
        ++i;
    }

    i = 0;
    cout << "_________________USED_UNIT______________"<< endl;
    for (auto iter = used_list.begin(); iter != used_list.end(); ++iter) {
        cout << "unit "<< i<< ":" << "offset: " << iter->offset << " size: " << iter->size<< endl;
        cout << "       "<< used_list.begin()->offset<< endl;
        ++i;
    }

}