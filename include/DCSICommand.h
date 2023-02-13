/**
 * @file DCSICommand.h
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
#ifndef DCSICommand_h
#define DCSICommand_h

#include <Arduino.h>
#include <DCSIconfig.h>
#include <DCSIlog.h>
#include <Queue.h>


// to be changed according to the numnr of commands
// defined in the cpp file
#define MAX_COMMANDS 3      // The number of commands the interface supports
#define MAX_PARAMS 5        // max number of parameters for a handler
#define MAX_NAME_LENGTH 4   // max length of a command name

// Types of parameters allowed in a command for the CommandStation and/or NetworkStation
enum class paramType
{
    NUM_T, // numbers
    STR_T  // strings
};

using CommandParams = Queue<char *, MAX_PARAMS + 1>;
using CommandHandler = int (*)(paramType &, CommandParams &);

// forward declaration of the Command
class Command;

class Commands
{
private:
    static char* CommandNames[MAX_COMMANDS];
    static Command* CommandRef[MAX_COMMANDS];
    static int count;
    static CommandParams pq;
    static void prepare(char *cmd); // once a command recieved / prepare for execution i.e.
                                    // fill the queue with the command found and the
                                    // ptr to the parameters represented as strings
public:
    static void run(const char *cmd);
    static CommandParams *getCommandParams();
    static void insert(Command *c);
    static Command *find(char * c);

    Commands() {
        count = 0;
    }

};

class Command
{
private:
    char name[MAX_NAME_LENGTH]; // has a name max 3 char plus terminator
    paramType pt[MAX_PARAMS];
    CommandHandler f;
    Command &self() { return *this; }
    // recursivley handle the parameter types of the command as provided
    // by the constructor
    int i = 0;                      // i+1 == number of parameters the queue should hold excluding 
                                    // the first element in the queue which will be the command
                                    
    void processTypes(){};
    template <typename T1, typename... T>
    void processTypes(const T1 &t1, const T &...t)
    {
        pt[i++] = t1;
        processTypes(t...);
    }

public:

    auto exec(CommandParams *p) -> void
    {
        // sanity check: Does the parameter type definition hold as many entries as we have in the queue
         if (p->size() == (size_t)(i)) {
            f(*pt,*p);
         } else {
            ERR(F("#%d of parameter types do not correpsond to provided #%d of parameter values" CR), p->size(), i+1);
         }
    }
    char *getName()
    {
        return name;
    }
    template <typename... Args>
    Command(char *n, CommandHandler c, Args... p) /* give it a hashmap to be added to  */
    {
        strncpy(name, n, 3); // set the name
        processTypes(p...);  // initalize the array with the types of the arguments
        f = c;               // set the handler for the command
        // fill the arrays in the map
        Commands::insert(&self());
    };
};

extern Commands Cmds;
#endif