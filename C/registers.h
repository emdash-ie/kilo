#pragma once

typedef struct Register Register;

struct Register {
  char *content;
  Register *previous;
};
