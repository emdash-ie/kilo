#pragma once

#include "int.h"
#include "ListListPaneRow.h"
#include "DisplayRow.h"

typedef struct DisplayRow DisplayRow;

#define n 4
#define a DisplayRow
#define b int
#define c int
#define d List(List(PaneRow))
#include "MakeLinkedList.h"
