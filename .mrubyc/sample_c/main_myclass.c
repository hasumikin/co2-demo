/*
 * Sample Main Program
 */

#include <stdio.h>
#include <stdlib.h>
#include "mrubyc.h"

#define MEMORY_SIZE (1024*30)
static uint8_t memory_pool[MEMORY_SIZE];

uint8_t * load_mrb_file(const char *filename)
{
  FILE *fp = fopen(filename, "rb");

  if( fp == NULL ) {
    fprintf(stderr, "File not found\n");
    return NULL;
  }

  // get filesize
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // allocate memory
  uint8_t *p = malloc(size);
  if( p != NULL ) {
    fread(p, sizeof(uint8_t), size, fp);
  } else {
    fprintf(stderr, "Memory allocate error.\n");
  }
  fclose(fp);

  return p;
}


// Sample code for making a mruby/c class
static void c_myclass_func(mrb_vm *vm, mrb_value *v, int argc)
{
  printf("MyClass - func\n");
}

void make_class(mrb_vm *vm)
{
  mrb_class *cls = mrbc_define_class(vm, "MyClass", mrbc_class_object);
  mrbc_define_method(vm, cls, "func", c_myclass_func);
}



void mrubyc(uint8_t *mrbbuf)
{
  struct VM *vm;

  mrbc_init_alloc(memory_pool, MEMORY_SIZE);
  init_static();

  vm = mrbc_vm_open(NULL);
  if( vm == 0 ) {
    fprintf(stderr, "Error: Can't open VM.\n");
    return;
  }

  if( mrbc_load_mrb(vm, mrbbuf) != 0 ) {
    fprintf(stderr, "Error: Illegal bytecode.\n");
    return;
  }

  mrbc_vm_begin(vm);

  make_class(vm);

  mrbc_vm_run(vm);
  mrbc_vm_end(vm);
  mrbc_vm_close(vm);
}


int main(int argc, char *argv[])
{
  if( argc != 2 ) {
    printf("Usage: %s <xxxx.mrb>\n", argv[0]);
    return 1;
  }

  uint8_t *mrbbuf = load_mrb_file( argv[1] );
  if( mrbbuf == 0 ) return 1;

  mrubyc( mrbbuf );
  free( mrbbuf );

  return 0;
}
