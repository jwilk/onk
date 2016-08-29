/**
 * Copyright © 2005-2013 Jakub Wilk <jwilk@jwilk.net>.
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
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>

#include "keylist.h"

int compare_keylist_item(const void *a, const void *b)
{
  return strcmp(((keylist_item_t*)a)->name, ((keylist_item_t*)b)->name);
}

static void invalid_config(unsigned int n)
{
  fprintf(stderr, "Invalid configuration at line %u\n", n);
  exit(EXIT_FAILURE);
}

static void fail()
{
  perror(NULL);
  exit(EXIT_FAILURE);
}

int main(void)
{
  char *fname = NULL, *tmp;
  size_t junk;

  if (getline(&fname, &junk, stdin) == -1)
  {
    perror(NULL);
    return EXIT_FAILURE;
  }
  tmp = strchr(fname, '\n');
  if (tmp != NULL)
    *tmp = '\0';

  bool conf_grab = false;
  bool conf_daemon = false;
  char *triggers[KEY_MAX + 1];
  memset(triggers, 0, sizeof triggers);
  for (int n = 1; ; n++)
  {
    char *line = NULL, *keyname, *command;
    if (getline(&line, &junk, stdin) == -1)
      break;
    if (*line == '#')
    {
      free(line);
      continue;
    }
    if (*line == ':')
    {
      tmp = strchr(line, '\n');
      if (tmp != NULL)
        *tmp = '\0';
      char *option = line + 1;
      if (strcmp(option, "grab") == 0)
        conf_grab = true;
      else if (strcmp(option, "daemon") == 0)
        conf_daemon = true;
      free(line);
      continue;
    }
    tmp = strchr(line, '\t');
    if (tmp == NULL)
      invalid_config(n);
    assert(tmp != NULL);
    *tmp = '\0';
    command = tmp + 1;
    keyname = line;

    int key, key2 = -1;
    if (sscanf(keyname, "%d..%d", &key, &key2) < 1)
    {
      keylist_item_t goal = { .name = keyname };
      keylist_item_t *result =
        bsearch(&goal, keylist, sizeof keylist / sizeof (keylist_item_t), sizeof (keylist_item_t), compare_keylist_item);
      if (result == NULL)
        invalid_config(n);
      assert(result != NULL);
      key = result->value;
    }
    if (key < 0 || key > KEY_MAX || key2 > KEY_MAX)
      invalid_config(n);
    do
      triggers[key++] = command;
    while (key2 > 0 && key <= key2);
  }

  struct input_event ev;
  FILE *file = fopen(fname, "r");
  if (file == NULL)
  {
    perror(fname);
    return EXIT_FAILURE;
  }
  if (conf_grab && ioctl(fileno(file), EVIOCGRAB, 1) == -1)
    fail();
  if (conf_daemon && (daemon(0, 0) == -1 || nice(NZERO / 2) == -1))
    fail();
  while (fread(&ev, sizeof ev, 1, file) > 0)
  {
    if (!ev.value || ev.type != EV_KEY || ev.code > KEY_MAX)
      continue;
    if (triggers[ev.code])
      system(triggers[ev.code]);
  }
  if (fclose(file) != 0)
    fail();
  return EXIT_SUCCESS;
}

/* vim:set ts=2 sts=2 sw=2 et: */
