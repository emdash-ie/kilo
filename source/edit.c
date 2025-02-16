#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"

struct String editTypeToString(enum EditType et) {
  switch (et) {
    case InsertText: return makeString("InsertText");
    case DeleteText: return makeString("DeleteText");
  };
}

struct String insertArgumentsToString(struct InsertArguments a) {
  size_t length = strlen(a.text);
  size_t total_length = length + 27;
  char *result = malloc(sizeof(char) * (total_length + 1));
  sprintf(result, "InsertArguments { text = %s }", a.text);
  return (struct String){.s = result, .length = total_length};
}

struct String deleteArgumentsToString(struct DeleteArguments d) {
  struct String s = objectToString(d.object);
  size_t total_length = s.length + 29;
  char *result = malloc(sizeof(char) * (total_length + 1));
  sprintf(result, "DeleteArguments { object = %s }", s.s);
  free(s.s);
  return (struct String){.s = result, .length = total_length};
}

struct String editToString(struct Edit e) {
  struct String et = editTypeToString(e.type);
  struct String args;
  char *field;
  switch (e.type) {
    case InsertText:
      args = insertArgumentsToString(e.insert);
      field = "insert";
      break;
    case DeleteText:
      args = deleteArgumentsToString(e.delete);
      field = "delete";
      break;
  }
  int resultLength = 27 + et.length + args.length;
  char *result = malloc(sizeof(char) * (resultLength + 1));
  sprintf(
    result,
    "Edit { type = %s, %s = %s }",
    et.s,
    field,
    args.s
  );
  free(args.s);
  return (struct String){.s = result, .length = resultLength};
}

struct String objectTypeToString(enum ObjectType ot) {
  switch (ot) {
    case Character: return (struct String){.s = "Character", .length = 9};
    case Word: return (struct String){.s = "Word", .length = 4};
    case Line: return (struct String){.s = "Line", .length = 4};
    case Paragraph: return (struct String){.s = "Paragraph", .length = 9};
    case Page: return (struct String){.s = "Page", .length = 4};
    case Buffer: return (struct String){.s = "Buffer", .length = 6};
  }
}

struct String objectToString(struct Object o) {
  struct String ot = objectTypeToString(o.type);
  size_t length = 18 + ot.length;
  char *s = malloc(sizeof(char) * (length + 1));
  sprintf(s, "Object { type = %s }", ot.s);
  return (struct String){.s = s, .length = length};
}

struct String navigationTypeToString(enum NavigationType nt) {
  switch (nt) {
    case ToStartOf: return (struct String){.s = "ToStartOf", .length = 9};
    case ToEndOf: return (struct String){.s = "ToEndOf", .length = 7};
    case ToNext: return (struct String){.s = "ToNext", .length = 6};
    case ToPrevious: return (struct String){.s = "ToPrevious", .length = 10};
  }
}

struct String navigationToString(struct Navigation n) {
  struct String nt = navigationTypeToString(n.type);
  struct String o = objectTypeToString(n.objectType);
  size_t length = 37 + nt.length + o.length;
  char *s = malloc(sizeof(char) * (length + 1));
  sprintf(s, "Navigation { type = %s, objectType = %s }", nt.s, o.s);
  return (struct String){.s = s, .length = length};
}
