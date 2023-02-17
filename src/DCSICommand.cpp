/**
 * @file DCSICommand.cpp
 * @author Gregor Baues
 * @brief 
 * @version 0.1
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 *
 * This is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * See the GNU General Public License for more details <https://www.gnu.org/licenses/>
 * 
 */

#include <Queue.h>
#include <DCSIlog.h>
#include <DCSICommand.h>

Queue<char *, MAX_PARAMS + 1> Commands::pq;
char *Commands::CommandNames[MAX_COMMANDS];
Command *Commands::CommandRef[MAX_COMMANDS];
int Commands::count;

// fwd decl
void removeChars(char *str, char *remove);

void Commands::prepare(char *cmd)
{

  char *token;
  char *rest = cmd;

  while ((token = strtok_r(rest, " ", &rest)))
  {
    TRC(F("Pushing Token: %s" CR), token);
    Cmds.pq.push(token);
  }
}
CommandParams *Commands::getCommandParams() {
  return &pq;
}
void Commands::insert(Command *c)
{
  if (count == MAX_COMMANDS)
    return; // array is full cant insert anything
  Commands::CommandNames[count] = (char *)c->getName();
  Commands::CommandRef[count] = c;
  Commands::count++;
  return;
}
void Commands::run(const char *cmd)
{

  char buffer[strlen(cmd)];
  strcpy(buffer, cmd);
  removeChars(buffer, (char *)"<>!");

  prepare(buffer);
  const char *c = pq.pop(); // command the rest of the params are still in the queue

  // get the command from the Map
  Command *cm = Commands::find((char *)c);

  if (cm != nullptr)
  {
    cm->exec(Commands::getCommandParams()); // execute the command found
  }
  else
  {
    ERR(F("Command %s unknown" CR), c);
    pq.clear(); // clear the queue
    return;
  }
}
Command *Commands::find(char *c)
{
  int i = 0;
  while (strcmp(CommandNames[i], c) != 0)
  {
    i++;
    if (i == MAX_COMMANDS)
      return nullptr;
  }
  return CommandRef[i];
}

/*--------------------------------------------------------*/
// Handler functions for the commands
/*--------------------------------------------------------*/
int handleLLV(paramType &ptlist, CommandParams &p)
{
  int ll = atoi(p.pop());
  INFO(F("LogLevel changed from %d to %d" CR), dccLog.getLevel(), ll);
  dccLog.setLevel(ll);
  p.clear(); // clear the queue; not necessary if we handle the command properly
  return 1;
}

int handleDiag(paramType &ptlist, CommandParams &p)
{
  INFO(F("DIAG messages will be send to client %d" CR), atoi(p.pop()));  
  p.clear(); // clear the queue; not necessary if we handle the command properly
  return 1;
}

/*--------------------------------------------------------*/
// Declare Commands we understand
/*--------------------------------------------------------*/
/**
 * @brief send the diagnostcs of the CommandStation to the client provided in the
 * message e;g. <! diag 1>. When the comm part of on the CS sends a diag message the
 * client will be set to the id provided here.
 */
static const Command diag((char *)"diag", handleDiag, paramType::NUM_T);
/**
 * @brief set the loglevel on the CommandStation
 *
 */
static const Command _logl((char *)"llv", handleLLV, paramType::NUM_T);


// Helper functions
void removeChars(char *str, char *remove)
{
  int strIndex = 0; // removeIndex;
  for (size_t i = 0; i < strlen(str); i++)
  {
    bool found = false;
    for (size_t j = 0; j < strlen(remove); j++)
      if (str[i] == remove[j])
      {
        found = true;
        break;
      }
    if (!found)
      str[strIndex++] = str[i];
  }
  str[strIndex] = '\0';
}