#include "string.h"

enum ObjectType { Character, Word, Line, Paragraph, Page, Buffer };

struct Object {
  enum ObjectType type;
};

enum NavigationType
  { ToStartOf
  , ToEndOf
  , ToNext
  , ToPrevious
  };

struct Navigation {
  enum NavigationType type;
  enum ObjectType objectType;
};

enum EditType
  { InsertText
  , DeleteText
  };

struct InsertArguments {
  char *text;
};

struct DeleteArguments {
  struct Object object;
};

struct Edit {
  enum EditType type;
  union { struct InsertArguments insert; struct DeleteArguments delete; };
};


struct String editTypeToString(enum EditType et);
struct String insertArgumentsToString(struct InsertArguments a);
struct String deleteArgumentsToString(struct DeleteArguments d);
struct String editToString(struct Edit e);
struct String objectTypeToString(enum ObjectType ot);
struct String objectToString(struct Object o);
struct String navigationTypeToString(enum NavigationType nt);
struct String navigationToString(struct Navigation n);
