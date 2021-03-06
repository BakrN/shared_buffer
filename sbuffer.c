/** 
 * \author Abubakr Nada 
 * Last Name: Nada 
 * First Name: Abubakr 
 * Student Number: r0767316   
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include "sbuffer.h"
#include <memory.h>

typedef struct {
    sbuffer_t* buffer; 
    sensor_data_t* data ;
}
sbuffer_data ; 
/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */

/**
 * a structure to keep track of the buffer
 */

int sbuffer_init(sbuffer_t **buffer){
    sbuffer_t *ptr = malloc(sizeof(sbuffer_t));
    ptr->iterators = NULL;
    ptr->reader_thread_count = 0;
    ptr->terminate_threads = 0 ; 

    ptr->map = umap_create(sbuffer_free_entry, sbuffer_add_table_entry, NULL, NULL);
    pthread_mutex_init(&(ptr->sbuffer_edit_mutex), NULL);
    pthread_cond_init(&(ptr->sbuffer_element_added), NULL);

    *buffer = ptr;
    if (*buffer == NULL)
    {
        return SBUFFER_FAILURE;
    }

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer){
    if (buffer == NULL || *buffer == NULL)
    {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&((*buffer)->sbuffer_edit_mutex));
    umap_destroy((*buffer)->map);
    pthread_mutex_unlock(&((*buffer)->sbuffer_edit_mutex));
    for (int i = 0 ; i < (*buffer)->reader_thread_count; i++){// free iterators
        free((*buffer)->iterators[i]); 
    }
    free((*buffer)->iterators); 
    
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

char sbuffer_wait_for_data(sbuffer_t *buffer, int thread_id){
    
    sbuffer_iterator *iterator = NULL;
    for (int i = 0; i < buffer->reader_thread_count; i++)
    {
        if (buffer->iterators[i]->thread_id == thread_id)
        {
            iterator = buffer->iterators[i];
            break;
        }
    }
    if (!iterator)
    {
        // reader thread hasn't subscribed ;
        return 1;
    }
    iterator->entry = get_next(buffer, thread_id); 
    pthread_mutex_lock(&(buffer->sbuffer_edit_mutex));

    while (buffer->terminate_threads == 0 && iterator->entry == NULL)
    {
        
        pthread_cond_wait(&(buffer->sbuffer_element_added), &(buffer->sbuffer_edit_mutex)); // wakes up with locked mutex
        iterator->entry = get_next(buffer, thread_id);
        
    }
    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex));
    
    if (buffer->terminate_threads && iterator->entry == NULL)
    { // woken up due to termination

        
        return 1;
    }

    return 0;
}
int sbuffer_get_entry_tbr(sbuffer_t* buffer, sbuffer_table_entry* entry, int thread_id){
    for (int i = 0; i <buffer->reader_thread_count; i++){
        if(entry->to_be_read[i]->thread_id == thread_id) {
            return entry->to_be_read[i]->tbr_count; 
        }
    }
    return -1 ; 
}
void sbuffer_reader_subscribe(sbuffer_t *buffer, int thread_id)
{
    pthread_mutex_lock(&(buffer->sbuffer_edit_mutex)); 
    buffer->reader_thread_count++;
    buffer->iterators =  realloc(buffer->iterators, buffer->reader_thread_count * sizeof(sbuffer_iterator *));
    (buffer->iterators)[buffer->reader_thread_count - 1] = malloc(sizeof(sbuffer_iterator));
    (buffer->iterators)[buffer->reader_thread_count - 1]->entry = NULL;
    (buffer->iterators)[buffer->reader_thread_count - 1]->thread_id = thread_id;

    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex)); 
}

void sbuffer_reader_unsubscribe(sbuffer_t *buffer, int thread_id)
{
    // lock mutex so info is added until the reader thread's data is deleted 
    pthread_mutex_lock(&(buffer->sbuffer_edit_mutex)); 
    sbuffer_table_entry* entry; 
    buffer->reader_thread_count--; 
    for (int i = 0 ; i < buffer->map->capacity; i++){
        entry = (sbuffer_table_entry*)umap_get_entry_by_index(buffer->map, i); 
        if(entry){
     
            for (int j = 0 ; j < entry->tbr_array_size; j++){
                if (entry->to_be_read[i]->thread_id == thread_id){
                    for (int k = j; k < entry->tbr_array_size-1; k++){
                        entry->to_be_read[k] = entry->to_be_read[k+1]; 

                    }
                    entry->tbr_array_size--; 
                    free(entry->to_be_read[entry->tbr_array_size]); 
                    entry->to_be_read = realloc(entry->to_be_read, sizeof(sbuffer_entry_toberead* )*(entry->tbr_array_size)); 
                    break; 
                }
            }
  
        }
    }
    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex)); 
}


int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data)
{
    sbuffer_table_entry* entry = umap_get_entry_by_key( buffer->map, data->id); 
    if ( entry ){

    sbuffer_data args  ; 
    args.buffer = buffer ; 
    args.data = data; 
            pthread_mutex_lock(&(buffer->sbuffer_edit_mutex));
            
        umap_add_to_entry(buffer->map, &args , data->id); 
    }
    else{
        // create entry 
        sbuffer_table_entry *new_entry = malloc(sizeof(sbuffer_table_entry));
        new_entry->key = data->id;
        new_entry->list = dpl_create(NULL, sbuffer_listelement_free, NULL);
        new_entry->tbr_array_size = buffer->reader_thread_count; 
        dpl_insert_at_index(new_entry->list, (void *)data, 0, 0);
        new_entry->to_be_read = malloc(buffer->reader_thread_count * sizeof(sbuffer_entry_toberead *));
        for (int i = 0; i < buffer->reader_thread_count; i++)
        {
            // add all reader thread ids and count to new entry
            if (!(buffer->iterators[i])->entry)
            {
                // iterator pointing to nothing
                buffer->iterators[i]->entry = new_entry;
            }
            new_entry->to_be_read[i] = malloc(sizeof(sbuffer_entry_toberead));
            new_entry->to_be_read[i]->thread_id = buffer->iterators[i]->thread_id;
            new_entry->to_be_read[i]->tbr_count = 1;
    
        }
            pthread_mutex_lock(&(buffer->sbuffer_edit_mutex));
        umap_add_new(buffer->map,new_entry, data->id); 
    }

 
    pthread_cond_broadcast(&(buffer->sbuffer_element_added)); // wake up other threads if they're asleep
    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex));

    return 0;
}

    /**
     * @brief Returns the iterator with the indicated thread_id . NULL is returned if ID isin't found in buffer. 
     * 
     * @param buffer sbuffer_t ptr 
     * @param thread_id Thread ID
     * @return sbuffer_iterator* 
     */
    sbuffer_iterator* sbuffer_iter(sbuffer_t* buffer, int thread_id){
        for(int i = 0 ; i < buffer->reader_thread_count; i++){
            if(buffer->iterators[i]->thread_id == thread_id){
                return buffer->iterators[i]; 
            }
        }
        return NULL; 
    }

int sbuffer_add_table_entry(void *entry , void *arg)
{
    sbuffer_data * args = (sbuffer_data* )arg; 

    sensor_data_t *data = args->data;
    sbuffer_table_entry *ptr = (sbuffer_table_entry *)(entry); 
    dpl_insert_at_index(ptr->list, data, 0, 0); // not copied
    for (int i = 0; i < ptr->tbr_array_size; i++)
       {
                if (!(args->buffer->iterators[i])->entry)
                {
                    // iterator pointing to nothing will now point to new entry 
                    args->buffer->iterators[i]->entry = ptr;
                }
                ptr->to_be_read[i]->tbr_count += 1;
            }
     
        return 0;
    }

    void sbuffer_free_entry(void *entry)
    {
        sbuffer_table_entry *ptr = (sbuffer_table_entry *)entry;
        dpl_free(&ptr->list, 1);
        ptr->list = NULL;
        for(int i =0 ;i < ptr->tbr_array_size; i++ ){
    
            free(ptr->to_be_read[i]) ; 
        }
        free(ptr->to_be_read); 
        free(ptr);
        
        ptr = NULL;
    }

    void sbuffer_listelement_free(void **element)
    {
        free(*element); // frees sensor_data_id pointer;
        *element = NULL;
    }

    sbuffer_table_entry *get_next(sbuffer_t * buffer, int thread_id)
    {
        sbuffer_iterator* ptr = NULL ; // iterator of specific thread_id; 
        for(int i = 0; i < buffer->reader_thread_count; i++){
            if (buffer->iterators[i]->thread_id == thread_id){
                ptr = buffer->iterators[i]; 
                      if(ptr->entry && sbuffer_get_entry_tbr(buffer, ptr->entry, thread_id)>0){ // iterator was se when entering new data into buffer 
            return ptr->entry; 
        }
                break; 
            }
        } 
     
        if(ptr){ // reader thread id found 
  
 

             // iterator ptr needs data because it's current null 
    unordered_map* map = buffer->map; 
    
        
        sbuffer_entry_toberead* tbr =NULL; //thread specific count
        
        for(int i =0; i < map->capacity; i++){
         sbuffer_table_entry* entry = umap_get_entry_by_index(map, i); 
        if(!entry) continue; 
  

        if(entry){ // sbuffer table entry not null 
            for (int j = 0; j < buffer->reader_thread_count; j++){
                if(entry->to_be_read[j]->thread_id == thread_id){
                    tbr = entry->to_be_read[j]; 
                }
        }
        if(tbr){
            if(tbr->tbr_count > 0 ){
                ptr->entry = entry; 
                return ptr->entry; 
            }
            } 
        } 
        }
        ptr->entry = NULL ;
        }
        
        return NULL; // no data found 
    }


    void sbuffer_update_iter(sbuffer_t * buffer, sbuffer_iterator* iter , int count){
        
       
            sbuffer_entry_toberead* tbr = NULL ; // pointer to tbr counter for specific thread id 
            for (int i = 0 ; i < buffer->reader_thread_count; i++){
                if(iter->entry->to_be_read[i]->thread_id == iter->thread_id){
                    tbr = iter->entry->to_be_read[i]; 
                    break; 
                }
            }

            pthread_mutex_lock(&buffer->sbuffer_edit_mutex);
            if(tbr){
            
    
                tbr->tbr_count -= count;
            }
            char remaining_threads_to_read = buffer->reader_thread_count; 
            for(int i =0 ; i < buffer->reader_thread_count; i++){
                if(iter->entry->to_be_read[i]->tbr_count ==0){
                    remaining_threads_to_read--; 
                }
            }
            if (remaining_threads_to_read==0)
            {
                // you can free list memory
                while (iter->entry->list->head)
                { // remove elements
                    iter->entry->list = dpl_remove_at_index(iter->entry->list, 0, 1);
                }
                
            }

            pthread_mutex_unlock(&buffer->sbuffer_edit_mutex);
        
    
    }
void sbuffer_wakeup_readerthreads(sbuffer_t* buffer){
     pthread_mutex_lock(&(buffer->sbuffer_edit_mutex)); 
    pthread_cond_broadcast(&(buffer->sbuffer_element_added )); // wake up other threads if they're asleep 
    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex)); 
}

void sbuffer_threadsleep(sbuffer_t* buffer, int thread_id ){
    pthread_mutex_lock(&(buffer->sbuffer_edit_mutex)); 
    pthread_cond_wait(&(buffer->sbuffer_element_added), &(buffer->sbuffer_edit_mutex)); // wakes up with locked mutex
    pthread_mutex_unlock(&(buffer->sbuffer_edit_mutex)); 
}
