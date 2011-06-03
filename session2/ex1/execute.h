#ifndef EXECUTE_H
#define EXECUTE_H

#include "command.h"

extern void executeCommand(struct command *c);
extern void waitForChildren(struct command *c);

#endif /* EXECUTE_H */
