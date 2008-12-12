/**
 * Copyright © 2005, 2006, 2007, 2008 Jakub Wilk <ubanus@users.sf.net>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/input.h>
#include "keylist.h"

static int compare_keylist_item(const void *a, const void *b)
{
  return strcmp(((keylist_item_t*)a)->name, ((keylist_item_t*)b)->name);
}

static inline void send_event(FILE *file, uint16_t type, uint16_t code, int32_t value)
{
  struct input_event ev;
  gettimeofday(&ev.time, NULL);
  ev.type = type;
  ev.code = code;
  ev.value = value;
  if (fwrite(&ev, sizeof(struct input_event), 1, file) != 1)
  {
    perror(NULL);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv)
{
  char *fname = "/dev/input/event0";
  if (argc > 1)
    fname = argv[1];

  FILE *file = fopen(fname, "w");
  if (!file)
  {
    perror(fname);
    return EXIT_FAILURE;
  }
 
  char *str = NULL;
  ssize_t strlen;
  size_t buflen = 0;
  
  while ((strlen = getline(&str, &buflen, stdin)) != -1)
  {
    int key;
    if (strlen > 0 && str[strlen - 1] == '\n')
      str[strlen - 1] = '\0';
    if (sscanf(str, "%d", &key) != 1)
    {
      keylist_item_t goal = { .name = str };
      keylist_item_t *result =
        bsearch(&goal, keylist, sizeof keylist / sizeof (keylist_item_t), sizeof (keylist_item_t), compare_keylist_item); 
      if (result == NULL)
        continue;
      key = result->value;
    } 
    if (key < 0 || key > KEY_MAX)
      continue;
    send_event(file, EV_KEY, key, 1);
    send_event(file, EV_SYN, SYN_REPORT, 0);
    send_event(file, EV_KEY, key, 0);
    send_event(file, EV_SYN, SYN_REPORT, 0);
    fflush(file);
  }
  fclose(file);
  return EXIT_SUCCESS;
}

/* vim:set ts=2 sw=2 et: */
