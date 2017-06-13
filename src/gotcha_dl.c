#include "gotcha_dl.h"
#include "tool.h"
#include "libc_wrappers.h"
#include "elf_ops.h"

static void*(*orig_dlopen)(const char* filename, int flags);
static void*(*orig_dlsym)(void* handle, const char* name);
struct rev_iter{
  struct binding_t* data;
  void* next;
};
struct rev_iter* get_reverse_tool_iterator(struct binding_t* in){
  struct rev_iter* rev_builder = (struct rev_iter*)malloc(sizeof(struct rev_iter));
  rev_builder->next = NULL;
  struct rev_iter* rever;
  struct binding_t* tool_iter = in;
  for(;tool_iter!=NULL;tool_iter = tool_iter->next_binding){
    printf("Testing print %s\n",tool_iter->tool->tool_name);
    rev_builder->data = tool_iter;
    rever =  (struct rev_iter*)malloc(sizeof(struct rev_iter));
    rever->next = rev_builder;
    rev_builder = rever;
  }
  return rev_builder;
}
void* dlopen_wrapper(const char* filename, int flags){
  void* handle = orig_dlopen(filename,flags);
  struct binding_t* tool_iter = get_bindings();
  struct binding_t* iter_helper = NULL;
  onlyFilterLast();
  for(;tool_iter!=NULL;tool_iter = tool_iter->next_binding){
      gotcha_wrap(tool_iter->user_binding,tool_iter->user_binding_size,tool_iter->tool->tool_name);
  }
  restoreLibraryFilterFunc();
  return handle;
}
void* dlsym_wrapper(void* handle, const char* symbol_name){
  struct binding_t* tool_iter = get_bindings();
  // TODO: free this chain
  struct rev_iter* rev = get_reverse_tool_iterator(tool_iter);
  for(;rev!=NULL;rev = rev->next){
    struct binding_t* tool_iter = rev->data;
    if(tool_iter){
      int loop = 0;
      for(loop=0;loop<tool_iter->user_binding_size;loop++){
        if(gotcha_strcmp(tool_iter->user_binding[loop].name,symbol_name)==0){
          return tool_iter->user_binding[loop].wrapper_pointer;
        }
      }
    }
  }
  return orig_dlsym(handle,symbol_name);
}

void handle_libdl(){
  static struct gotcha_binding_t dl_binds[] = {
    {"dlopen", dlopen_wrapper, &orig_dlopen},
    {"dlsym", dlsym_wrapper, &orig_dlsym}
  };     
  gotcha_wrap(dl_binds, 2, "gotcha");
}
