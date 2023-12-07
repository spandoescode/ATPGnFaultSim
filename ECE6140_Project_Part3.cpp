#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <tuple>

class Net
{

public:
    // Information about the Net
    int id;

    // Information about connected Gates
    std::vector<int> gates_into;
    int input;

    // Value that the net carries
    int value;

    // Flag to indicate the precense of a fault value D/Dbar
    int isFault;

    // Class constructor
    Net(int value, int _id, int _input)
    {
        value = value;
        id = _id;
        input = _input;
    }

    // Default constructor
    Net()
    {
        value = -1;
        id = -1;
        input = -1;
        isFault = -1;
    }
};

class Fault
{
public:
    int net_id;
    int value;

    Fault(int _net_id, int _value)
    {
        net_id = _net_id;
        value = _value;
    }

    Fault()
    {
        net_id = -1;
        value = -1;
    }

    bool operator<(const Fault &struct2) const
    {
        return std::tie(net_id, value) < std::tie(struct2.net_id, struct2.value);
    }

    bool operator==(const Fault &struct2) const
    {
        return std::tie(net_id, value) == std::tie(struct2.net_id, struct2.value);
    }
};

class Gate
{

public:
    // Information about the gate
    std::string type;
    int id;

    // Net information
    std::vector<Net> input_nets;
    Net output_net;

    // Default constructor
    Gate()
    {
        type = "";
        id = -1;
        input_nets = {};
    }

    // Class constructor for two input gates
    Gate(std::string _type, int _id, Net in1, Net in2, Net out)
    {
        type = _type;
        id = _id;

        input_nets.push_back(in1);
        input_nets.push_back(in2);
        output_net = out;
    }

    // Class constructor for one input gates
    Gate(std::string _type, int _id, Net in1, Net out)
    {
        type = _type;
        id = _id;

        input_nets.push_back(in1);
        output_net = out;
    }
};

// Function to compare two nets based on their ID
// Used for inserting net into the list of nets in order of the ID
bool compareById(const Net &net1, const Net &net2)
{
    return net1.id < net2.id;
}

// Returns the controlling value of the particular gate
int controlling_value(std::string type)
{
    if (type == "AND")
        return 0;
    else if (type == "NAND")
        return 0;
    else if (type == "OR")
        return 1;
    else if (type == "NOR")
        return 1;
    else
        return -3;
}

std::tuple<int, int> objective(int net_id, int net_val, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
{
    int l = -1, v = -2;

    // If the value of the net is unassigned
    if (net_list[net_id - 1].value == -1)
    {
        l = net_id;
        v = !net_val;
        return std::make_tuple(l, v);
    }

    // Find a gate from the D frontier, ie, a gate with an unassigned output and a fault value at the input
    for (int j = 0; j < gate_list.size(); ++j)
    {
        Gate g = gate_list[j];

        // If the gate has an unassigned value
        if (net_list[g.output_net.id - 1].value == -1)
        {
            if (g.input_nets.size() == 2)
            {
                // If the value is found and if the other input is unassigned
                if (net_list[g.input_nets[0].id - 1].isFault == 1)
                {
                    if (net_list[g.input_nets[1].id - 1].value == -1)
                    {
                        // Set the correct net id
                        int l = g.input_nets[1].id;

                        // Get the inversion of the gate
                        int c = !controlling_value(g.type);

                        return std::make_tuple(l, c);
                    }
                }
                else if (net_list[g.input_nets[1].id - 1].isFault == 1)
                {
                    if (net_list[g.input_nets[0].id - 1].value == -1)
                    {
                        // Set the correct net id
                        int l = g.input_nets[0].id;

                        // Get the inversion of the gate
                        int c = !controlling_value(g.type);

                        return std::make_tuple(l, c);
                    }
                }
            }
        }
    }

    return std::make_tuple(l, v);
}

// Map a desired objective to a PI assignment
std::tuple<int, int> backtrace(int net_id, int net_val, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
{
    // Original objective values
    int k = net_id;
    int v = net_val;

    // While the current net is not a PI
    while (net_list[k - 1].input != -1)
    {
        // For the input gate of the current net
        Gate g = gate_list[net_list[k - 1].input];

        // Get the inversion of the gate
        int i;
        if (g.type == "BUF" || g.type == "OR" || g.type == "AND")
        {
            i = 0;
        }
        else
        {
            i = 1;
        }

        // For an input of the gate with an unassigned value
        int j = -1;
        for (int m = 0; m < g.input_nets.size(); ++m)
        {
            if (g.input_nets[m].value == -1)
            {
                j = g.input_nets[m].id;
                break;
            }
        }

        // Update the value of v to track
        v = v ^ i;

        // Repeat the process for the input gate
        k = j;

        // If the backtrace fails
        if (k == -1)
        {
            break;
        }
    }

    return std::make_tuple(k, v);
}

void propagateFault(Gate g, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
{
    if (g.input_nets.size() == 1)
    {
        int in = g.input_nets[0].id;
        int out = g.output_net.id;

        // Directly propagate the fault
        net_list[out - 1].isFault = net_list[in - 1].isFault;
    }
    else
    {
        // If one of the inputs is a fault value and the other is a non controlling value, propagate the fault value to the output
        int in1 = g.input_nets[0].id;
        int in2 = g.input_nets[1].id;

        if (net_list[in1 - 1].isFault == 1)
        {
            if (net_list[in2 - 1].value != controlling_value(g.type))
            {
                net_list[g.output_net.id - 1].isFault = 1;
            }
        }

        if (net_list[in2 - 1].isFault == 1)
        {
            if (net_list[in1 - 1].value != controlling_value(g.type))
            {
                net_list[g.output_net.id - 1].isFault = 1;
            }
        }
    }
}

void evaluateGate(Fault target, Gate g, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
{
    // Temporary variables for calculation
    int inval1, inval2, outval;

    // out = in
    if (g.type == "BUF")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        outval = inval1;
    }
    // out = !in
    else if (g.type == "INV")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;

        if (inval1 == -1)
        {
            outval = inval1;
        }
        else
        {
            outval = !inval1;
        }
    }
    else if (g.type == "AND")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;

        if (inval1 == 0 || inval2 == 0)
        {
            outval = 0;
        }
        else
        {
            outval = inval1 & inval2;
        }
    }
    else if (g.type == "OR")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;

        if (inval1 == 1 || inval2 == 1)
        {
            outval = 1;
        }
        else
        {
            outval = inval1 | inval2;
        }
    }
    else if (g.type == "NAND")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;

        if (inval1 == 0 || inval2 == 0)
        {
            outval = 1;
        }
        else
        {
            outval = !(inval1 & inval2);
        }
    }
    else if (g.type == "NOR")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;

        if (inval1 == 1 || inval2 == 1)
        {
            outval = 0;
        }
        else
        {
            outval = !(inval1 | inval2);
        }
    }

    // Update the value in the gate list
    net_list[g.output_net.id - 1].value = outval;

    // Find all gates which have the net as an input
    for (int j = 0; j < gate_list.size(); ++j)
    {
        // For all the inputs of a given gate
        for (int k = 0; k < gate_list[j].input_nets.size(); ++k)
        {
            // For all the input which is the current net
            if (gate_list[j].input_nets[k].id == net_list[g.output_net.id - 1].id)
            {
                // Update the value of the net in the gate object list
                gate_list[j].input_nets[k].value = net_list[g.output_net.id - 1].value;
            }
        }
    }

    // Propagate the fault
    propagateFault(g, net_list, gate_list);

    // For the first run of PODEM
    if (g.output_net.id == target.net_id && net_list[g.output_net.id - 1].value != target.value)
    {
        // Create a fault on the output line
        net_list[g.output_net.id - 1].isFault = 1;
    }
}

void imply(Fault target, int net, int val, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
{
    // Set the PI to the given value
    net_list[net - 1].value = val;

    // If the current PI is the target fault site
    if (net == target.net_id)
    {
        net_list[net - 1].isFault = 1;
    }

    // Find all gates which have the net as an input
    for (int j = 0; j < gate_list.size(); ++j)
    {
        // For all the inputs of a given gate
        for (int k = 0; k < gate_list[j].input_nets.size(); ++k)
        {
            // For all the input which is the current net
            if (gate_list[j].input_nets[k].id == net_list[net - 1].id)
            {
                // Update the value of the net in the gate object list
                gate_list[j].input_nets[k].value = val;
            }
        }
    }

    // Create a stack for logic simulation
    std::stack<Gate> logic_stack;

    // Create a list which stores the ids of gates already added to the stack
    std::vector<int> added_to_stack;

    // Add all gates with both inputs assigned to a logic stack
    for (int i = 0; i < gate_list.size(); ++i)
    {
        // For the current gate
        Gate g = gate_list[i];

        // Flag to set if the gate is assigned
        bool isAssigned = true;
        bool isOneControlling = false;

        // check for unassigned outputs
        if (net_list[g.output_net.id - 1].value == -1)
        {
            // Either only one input is assigned and that input is the controlling value which does not carry the fault
            for (int j = 0; j < g.input_nets.size(); ++j)
            {
                if (g.input_nets[j].value == controlling_value(g.type) && net_list[g.input_nets[j].id - 1].isFault != 1)
                {
                    isOneControlling = true;
                    break;
                }
            }

            // Or both inputs are assigned
            for (int j = 0; j < g.input_nets.size(); ++j)
            {
                if (g.input_nets[j].value == -1)
                {
                    isAssigned = false;
                    break;
                }
            }
        }

        // If the gate has one input assigned
        if (isAssigned == true || isOneControlling == true)
        {
            // Flag to mark if the gate has already been added to the stack
            bool isAdded = false;

            // Check if the gate is not already in the stack
            for (int k = 0; k < added_to_stack.size(); ++k)
            {
                if (added_to_stack[k] == g.id)
                {
                    isAdded = true;
                    break;
                }
            }

            // If the gate was not added previously
            if (isAdded == false)
            {
                // Push the gate onto the stack
                logic_stack.push(g);
                added_to_stack.push_back(g.id);
            }
        }
    }

    // While the stack is not empty
    while (!logic_stack.empty())
    {
        // Fetch the top value from the stack
        Gate g = logic_stack.top();

        // Pop the stack
        logic_stack.pop();

        // If the inputs are valid
        int in1 = g.input_nets[0].id;
        int in2;

        if (g.input_nets.size() == 2)
        {
            in2 = g.input_nets[1].id;
        }

        if (net_list[in1 - 1].value != -1 && (g.type == "INV" | g.type == "BUF"))
        {
            // Evaluate the output of the gate
            evaluateGate(target, g, net_list, gate_list);
        }
        else if ((g.type == "AND" || g.type == "OR" || g.type == "NAND" || g.type == "NOR") && net_list[in1 - 1].value != -1 && net_list[in1 - 1].value == controlling_value(g.type))
        {
            // Evaluate the output of the gate
            evaluateGate(target, g, net_list, gate_list);
        }
        else if ((g.type == "AND" || g.type == "OR" || g.type == "NAND" || g.type == "NOR") && net_list[in2 - 1].value != -1 && net_list[in2 - 1].value == controlling_value(g.type))
        {
            // Evaluate the output of the gate
            evaluateGate(target, g, net_list, gate_list);
        }
        else if ((g.type == "AND" || g.type == "OR" || g.type == "NAND" || g.type == "NOR") && net_list[in1 - 1].value != -1 && net_list[in2 - 1].value != -1)
        {
            // Evaluate the output of the gate
            evaluateGate(target, g, net_list, gate_list);
        }

        // Find gates with all assigned inputs not already in the stack
        for (int i = 0; i < gate_list.size(); ++i)
        {
            // Fetch the number of inputs for the current gate
            int ins = gate_list[i].input_nets.size();

            // Flag to set if the gate is assigned
            bool isAssigned = true;
            bool isOneControlling = false;

            if (net_list[gate_list[i].output_net.id - 1].value == -1)
            {
                // Check if at least one input is assigned

                // Either only one input is assigned and that input is the controlling value
                for (int j = 0; j < gate_list[i].input_nets.size(); ++j)
                {
                    if (gate_list[i].input_nets[j].value == controlling_value(gate_list[i].type) && net_list[gate_list[i].input_nets[j].id - 1].isFault != 1)
                    {
                        isOneControlling = true;
                        break;
                    }
                }

                // Or both inputs are assigned
                for (int j = 0; j < ins; ++j)
                {
                    if (gate_list[i].input_nets[j].value == -1)
                    {
                        isAssigned = false;
                        break;
                    }
                }
            }

            // Flag to mark if the gate has already been added to the stack
            bool isAdded = false;

            if (isAssigned == true || isOneControlling == true)
            {
                // Check if the gate is not already in the stack
                for (int k = 0; k < added_to_stack.size(); ++k)
                {
                    if (added_to_stack[k] == gate_list[i].id)
                    {
                        isAdded = true;
                        break;
                    }
                }

                // If the gate was not added previously
                if (isAdded == false)
                {
                    // Push the gate onto the stack
                    logic_stack.push(gate_list[i]);
                    added_to_stack.push_back(gate_list[i].id);
                }
            }
        }
    }
}

int PODEM(Fault target, std::vector<Net> &net_list, std::vector<int> &output_list, std::vector<Gate> &gate_list)
{
    // Check if the error has reached a primary output
    for (int i = 0; i < output_list.size(); i++)
    {
        if (net_list[output_list[i] - 1].isFault == 1)
        {
            std::cout << "Fault propagated to output net " << output_list[i] << std::endl;
            return 1;
        }
    }

    // Call the objective function based on the target fault
    int obj_net, obj_val;
    std::tie(obj_net, obj_val) = objective(target.net_id, target.value, net_list, gate_list);

    // If the test is not possible, ie, if the objective is empty, return failure
    if (obj_net == -1 && obj_val == -2)
    {
        return 0;
    }

    // Backtrace using the objective recieved
    int set_net, set_val;
    std::tie(set_net, set_val) = backtrace(obj_net, obj_val, net_list, gate_list);

    // If the backtrace is empty, return failure
    if (set_net == -1)
    {
        return 0;
    }

    std::cout << "The objective set is " << obj_net << " " << obj_val << std::endl;
    std::cout << "The backtrack set is " << set_net << " " << set_val << std::endl;

    // If all goes well, imply the PI assignments found from the backtrace
    imply(target, set_net, set_val, net_list, gate_list);

    // Recursively call PODEM and check if the D frontier moves
    if (PODEM(target, net_list, output_list, gate_list) == 1)
    {
        return 1;
    }

    std::cout << "The new backtrack set is " << set_net << " " << !set_val << std::endl;

    // If PODEM fails, reverse the implication
    imply(target, set_net, !set_val, net_list, gate_list);

    // Check if the D frontier moves
    if (PODEM(target, net_list, output_list, gate_list) == 1)
    {
        return 1;
    }

    std::cout << "The final backtrack set is " << set_net << " -1" << std::endl;

    // IF PODEM fails again, imply the PI with value x
    imply(target, set_net, -1, net_list, gate_list);

    return 0;
}

int main()
{
    // Create the vectors to store the list of Gates and Nets
    std::vector<Gate> gate_list;
    std::vector<Net> net_list;

    // Create a vector to store the inputs and outputs of the circuit
    std::vector<int> input_list;
    std::vector<int> output_list;

    // Initialise file pointers for the input, output and netlist files
    std::ifstream fin;
    std::ifstream finput;
    std::ifstream ffault;
    std::ofstream foutput;

    // Select which file to open
    int file_set = 4;

    // Get the file to use from the user
    std::string filename;

    // Get the input from the user
    std::cout << "Enter the index of the file to use: ";
    std::cin >> file_set;

    // Open the set of input, output and netlist files according to the variable above
    switch (file_set)
    {
    case 0:
        filename = "s27.txt";
        fin.open("s27.txt");
        finput.open("i_s27.txt");
        foutput.open("o_s27.txt");
        ffault.open("f_s27.txt");
        break;

    case 1:
        filename = "s298f_2.txt";
        fin.open("s298f_2.txt");
        finput.open("i_s298f_2.txt");
        foutput.open("o_s298f_2.txt");
        ffault.open("f_s298f_2.txt");
        break;

    case 2:
        filename = "s344f_2.txt";
        fin.open("s344f_2.txt");
        finput.open("i_s344f_2.txt");
        foutput.open("o_s344f_2.txt");
        ffault.open("f_s344f_2.txt");
        break;

    case 3:
        filename = "s349f_2.txt";
        fin.open("s349f_2.txt");
        finput.open("i_s349f_2.txt");
        foutput.open("o_s349f_2.txt");
        ffault.open("f_s349f_2.txt");
        break;

    case 4:
        filename = "s344ff_2.txt";
        fin.open("s344ff_2.txt");
        finput.open("i_s344ff_2.txt");
        foutput.open("o_s344ff_2.txt");
        ffault.open("f_s344ff_2.txt");
        break;

    default:
        filename = "test.txt";
        fin.open("test.txt");
        finput.open("i_test.txt");
        foutput.open("o_test.txt");
        ffault.open("f_test.txt");
        break;
    }

    // Parsing the Netlist

    // Strings to store the input characters
    std::string line;
    std::string str1, str2, str3, str4;

    // Variables to store the net IDs
    int net_val1, net_val2, out_val;

    // Variable to store the count of the gates (and to give an id)
    int id = 0;

    // Till the end of file
    while (getline(fin, line))
    {
        std::stringstream ss(line);
        ss >> str1;

        // If GATE
        if (str1 == "INV" || str1 == "BUF")
        {
            // Get the rest of the parameters as input
            ss >> str2 >> str3;

            net_val1 = std::stoi(str2);
            out_val = std::stoi(str3);

            // Flag to mark if the net already exists
            bool exists = false;

            // Net object to pass to the gate constructor
            Net in1;

            // Check if the net already exists
            for (int i = 0; i < net_list.size(); ++i)
            {
                if (net_list[i].id == net_val1)
                {
                    // Update the gates_into list
                    net_list[i].gates_into.push_back(id);

                    // Make a copy of the existing net to pass to the gate constructor
                    in1 = net_list[i];

                    exists = true;
                    break; // Found a Net with the target id, no need to continue searching
                }
            }

            // If the net does not exist
            if (exists == false)
            {
                // If net doesn't exist, create a new object and insert into the array in a sorted format
                in1.gates_into.push_back(id);
                in1.id = net_val1;
                in1.input = -1;
                in1.value = -1;
                in1.isFault = 0;

                // If the vector is empty
                if (net_list.empty())
                {
                    net_list.push_back(in1);
                }
                // Else
                else
                {
                    auto it = std::lower_bound(net_list.begin(), net_list.end(), in1, compareById);
                    net_list.insert(it, in1);
                }
            }

            exists = false;

            // Net object to pass to the gate constructor
            Net out;

            // Check if the net already exists
            for (int i = 0; i < net_list.size(); ++i)
            {
                if (net_list[i].id == out_val)
                {
                    // Update the input variable of the gate
                    net_list[i].input = id;

                    // Make a copy of the existing net to pass to the gate constructor
                    out = net_list[i];

                    exists = true;
                    break; // Found a Net with the target id, no need to continue searching
                }
            }

            // If the net does not exist
            if (exists == false)
            {
                // If net doesn't exist, create a new object and insert into the array in a sorted format
                out.id = out_val;
                out.input = id;
                out.value = -1;
                out.isFault = 0;

                // If the vector is empty
                if (net_list.empty())
                {
                    net_list.push_back(out);
                }
                // Else
                else
                {
                    auto it = std::lower_bound(net_list.begin(), net_list.end(), out, compareById);
                    net_list.insert(it, out);
                }
            }

            // Create a gate object and instantiate with the above values
            Gate g(str1, id, in1, out);
            gate_list.push_back(g);

            id++;
        }
        else if (str1 == "AND" || str1 == "OR" || str1 == "NOR" || str1 == "NAND")
        {
            // Get the rest of the parameters as input
            ss >> str2 >> str3 >> str4;

            net_val1 = std::stoi(str2);
            net_val2 = std::stoi(str3);
            out_val = std::stoi(str4);

            // Flag to mark if the net already exists
            bool exists = false;

            // Net object to pass to the gate constructor
            Net in1;

            // Check if the net already exists
            for (int i = 0; i < net_list.size(); ++i)
            {
                if (net_list[i].id == net_val1)
                {
                    // Update the gates_into list
                    net_list[i].gates_into.push_back(id);

                    // Make a copy of the existing net to pass to the gate constructor
                    in1 = net_list[i];

                    exists = true;
                    break; // Found a Net with the target id, no need to continue searching
                }
            }

            // If the net does not exist
            if (exists == false)
            {
                // If net doesn't exist, create a new object and insert into the array in a sorted format
                in1.gates_into.push_back(id);
                in1.id = net_val1;
                in1.input = -1;
                in1.value = -1;
                in1.isFault = 0;

                // If the vector is empty
                if (net_list.empty())
                {
                    net_list.push_back(in1);
                }
                // Else
                else
                {
                    auto it = std::lower_bound(net_list.begin(), net_list.end(), in1, compareById);
                    net_list.insert(it, in1);
                }
            }

            exists = false;

            // Net object to pass to the gate constructor
            Net in2;

            // Check if the net already exists
            for (int i = 0; i < net_list.size(); ++i)
            {
                if (net_list[i].id == net_val2)
                {
                    // Update the gates_into list
                    net_list[i].gates_into.push_back(id);

                    // Make a copy of the existing net to pass to the gate constructor
                    in2 = net_list[i];

                    exists = true;
                    break; // Found a Net with the target id, no need to continue searching
                }
            }

            // If the net does not exist
            if (exists == false)
            {
                // If net doesn't exist, create a new object and insert into the array in a sorted format
                in2.gates_into.push_back(id);
                in2.id = net_val2;
                in2.input = -1;
                in2.value = -1;
                in2.isFault = 0;

                // If the vector is empty
                if (net_list.empty())
                {
                    net_list.push_back(in2);
                }
                // Else
                else
                {
                    auto it = std::lower_bound(net_list.begin(), net_list.end(), in2, compareById);
                    net_list.insert(it, in2);
                }
            }

            exists = false;

            // Net object to pass to the gate constructor
            Net out;

            // Check if the net already exists
            for (int i = 0; i < net_list.size(); ++i)
            {
                if (net_list[i].id == out_val)
                {
                    // Update the input variable of the net
                    net_list[i].input = id;

                    // Make a copy of the existing net to pass to the gate constructor
                    out = net_list[i];

                    exists = true;
                    break; // Found a Net with the target id, no need to continue searching
                }
            }

            // If the net does not exist
            if (exists == false)
            {
                // If net doesn't exist, create a new object and insert into the array in a sorted format
                out.id = out_val;
                out.input = id;
                out.value = -1;
                out.isFault = 0;

                // If the vector is empty
                if (net_list.empty())
                {
                    net_list.push_back(out);
                }
                // Else
                else
                {
                    auto it = std::lower_bound(net_list.begin(), net_list.end(), out, compareById);
                    net_list.insert(it, out);
                }
            }

            // Create a gate object and instantiate with the above values
            Gate g(str1, id, in1, in2, out);
            gate_list.push_back(g);

            id++;
        }
        // If INPUT
        else if (str1 == "INPUT")
        {
            int num;
            while (ss >> num && num != -1)
            {
                input_list.push_back(num);
            }
        }
        // If OUTPUT
        else if (str1 == "OUTPUT")
        {
            int num;
            while (ss >> num && num != -1)
            {
                output_list.push_back(num);
            }
        }
        // If Invalid character in netlist, do nothing
        else
        {
            std ::cout << "Invalid Input, will be ignored" << std::endl;
        }
    }

    // Print some basic information about the circuit
    std::cout << "File Name: " << filename << std::endl;
    std::cout << "Circuit Number " << file_set + 1 << ": " << std::endl;
    std::cout << "The circuit has " << gate_list.size() << " gates." << std::endl;
    std::cout << "The circuit has " << net_list.size() << " nets." << std::endl;
    std::cout << "The circuit has " << input_list.size() << " inputs." << std::endl;
    std::cout << "The circuit has " << output_list.size() << " outputs." << std::endl;

    // Create a list of faults to generate tests for
    std::vector<Fault> fault_list;

    std::string fnet, fval;

    // Parse the file containing the fault list
    while (getline(ffault, line))
    {
        std::stringstream ss(line);
        ss >> fnet >> fval;

        int _net_id = std::stoi(fnet);
        int _value = std::stoi(fval);

        Fault temp(_net_id, _value);
        fault_list.push_back(temp);
    }

    // For all faults in the fault list
    for (int i = 0; i < fault_list.size(); ++i)
    {
        // For the current fault
        Fault target(fault_list[i].net_id, fault_list[i].value);

        // Call PODEM on the fault
        int status = PODEM(target, net_list, output_list, gate_list);

        // If the test generation is successful
        if (status == 1)
        {
            // Print the value of the generated test(s) to a file
            for (int i = 0; i < input_list.size(); ++i)
            {
                if (net_list[input_list[i] - 1].value != -1)
                {
                    foutput << net_list[input_list[i] - 1].value;
                }
                else
                {
                    // Print X for an unassigned input
                    foutput << "X";
                }
            }
            foutput << std::endl;
        }
        else
        {
            // Print that the fault is undetectable
            std::cout << "The fault " << target.net_id << " stuck at " << target.value << " is undetectable." << std::endl;

            // Write to the file
            foutput << "Undetectable" << std::endl;
        }

        // Clear all the variables related to the simulation
        // Reset the value of all nets
        for (int j = 0; j < net_list.size(); j++)
        {
            net_list[j].value = -1;
            net_list[j].isFault = 0;
        }

        // Reset the values stored in the gate list
        for (int j = 0; j < gate_list.size(); j++)
        {
            gate_list[j].input_nets[0].value = -1;
            gate_list[j].input_nets[0].isFault = 0;
            gate_list[j].output_net.value = -1;
            gate_list[j].output_net.isFault = 0;

            if (gate_list[j].input_nets.size() == 2)
            {
                gate_list[j].input_nets[1].value = -1;
                gate_list[j].input_nets[1].isFault = 0;
            }
        }
    }

    // Close the files
    ffault.close();
    foutput.close();

    return 0;
}