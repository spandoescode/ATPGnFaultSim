#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

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

void evaluateGate(Gate &g, std::vector<Net> &net_list, std::vector<Gate> &gate_list)
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
        outval = !inval1;
    }
    else if (g.type == "AND")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        outval = inval1 & inval2;
    }
    else if (g.type == "OR")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        outval = inval1 | inval2;
    }
    else if (g.type == "NAND")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        outval = !(inval1 & inval2);
    }
    else if (g.type == "NOR")
    {
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        outval = !(inval1 | inval2);
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
}

void calculateFaultList(Gate &g, std::vector<Net> &net_list, int c, int inval1, int inval2, int correct_val, std::vector<Fault> *&fault_lists, std::vector<Fault> &global_fault_list, int *added_to_fault)
{
    // Create a list to store the result of the union
    std::vector<Fault> result;

    // Fetch the fault lists of the input nets
    std::vector<Fault> list1 = fault_lists[g.input_nets[0].id - 1];
    std::vector<Fault> list2 = fault_lists[g.input_nets[1].id - 1];

    // Sort the fault lists
    std::sort(list1.begin(), list1.end());
    std::sort(list2.begin(), list2.end());

    // If both inputs are controlling values
    if (inval1 == c && inval2 == c)
    {
        // Perform the intersection
        std::set_intersection(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(result));

    } // If only one input is a controlling value
    else if (inval1 == c && inval2 != c)
    {
        // Perform the set difference
        std::set_difference(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(result));
    }
    else if (inval2 == c && inval1 != c)
    {
        // Perform the set difference
        std::set_difference(list2.begin(), list2.end(), list1.begin(), list1.end(), std::back_inserter(result));

    } // If no input is a controlling value
    else
    {
        // Perform the union
        std::set_union(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(result));
    }

    // Check for the fault on the output line
    // Fetch the input value of the gate
    correct_val = net_list[g.output_net.id - 1].value;

    // Create parameters to search for
    int id = g.output_net.id;
    int val = !correct_val;

    // Check for the fault net stuck at !correct_val in the global fault list
    auto it = std::find_if(global_fault_list.begin(), global_fault_list.end(),
                           [id, val](const Fault &f)
                           { return f.net_id == id && f.value == val; });

    // If the fault was found
    if (it != global_fault_list.end())
    {
        // Create a fault object
        Fault f(id, val);

        // Union with the fault list of the input net
        std::vector<Fault> list1 = {f};
        std::vector<Fault> list2 = result;

        std::vector<Fault> temp_result;

        // Sort the vectors
        std::sort(list1.begin(), list1.end());
        std::sort(list2.begin(), list2.end());

        // Perform the union
        std::set_union(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(temp_result));

        // Add to the result
        result = temp_result;
    }

    // Add the result to the fault list of the net
    fault_lists[g.output_net.id - 1] = result;

    // Update the appropriate flag
    added_to_fault[g.output_net.id - 1] = 1;
}

void evaluateFaultList(Gate &g, std::vector<Net> &net_list, std::vector<Gate> &gate_list, std::vector<Fault> *fault_lists, std::vector<Fault> &global_fault_list, int *added_to_fault)
{
    // Temporary variables for calculation
    int correct_val;
    int inval1, inval2;
    int c, i;

    // Check the type of gate
    if (g.type == "BUF")
    {
        // Fetch the input value of the gate
        correct_val = net_list[g.input_nets[0].id - 1].value;

        // Create parameters to search for
        int id = g.output_net.id;
        int val = !correct_val;

        // Check for the fault net stuck at !correct_val in the global fault list
        auto it = std::find_if(global_fault_list.begin(), global_fault_list.end(),
                               [id, val](const Fault &f)
                               { return f.net_id == id && f.value == val; });

        // If the fault was found
        if (it != global_fault_list.end())
        {
            // Create a fault object
            Fault f(id, val);

            // Union with the fault list of the input net
            std::vector<Fault> list1 = {f};
            std::vector<Fault> list2 = fault_lists[g.input_nets[0].id - 1];

            std::vector<Fault> result;

            // Sort the vectors
            std::sort(list1.begin(), list1.end());
            std::sort(list2.begin(), list2.end());

            // Perform the union
            std::set_union(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(result));

            // Add to the fault list of the respective net
            fault_lists[g.output_net.id - 1] = result;

            // Update the appropriate flag
            added_to_fault[g.output_net.id - 1] = 1;
        }
    }
    else if (g.type == "INV")
    {
        // Fetch the input value of the gate
        correct_val = net_list[g.input_nets[0].id - 1].value;

        // Create parameters to search for
        int id = g.output_net.id;
        int val = correct_val;

        // Check for the fault net stuck at !correct_val in the global fault list
        auto it = std::find_if(global_fault_list.begin(), global_fault_list.end(),
                               [id, val](const Fault &f)
                               { return f.net_id == id && f.value == val; });

        // If the fault was found
        if (it != global_fault_list.end())
        {
            // Create a fault object
            Fault f(id, val);

            // Union with the fault list of the input net
            std::vector<Fault> list1 = {f};
            std::vector<Fault> list2 = fault_lists[g.input_nets[0].id - 1];

            std::vector<Fault> result;

            // Sort the vectors
            std::sort(list1.begin(), list1.end());
            std::sort(list2.begin(), list2.end());

            // Perform the union
            std::set_union(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(result));

            // Add to the fault list of the respective net
            fault_lists[g.output_net.id - 1] = result;

            // Update the appropriate flag
            added_to_fault[g.output_net.id - 1] = 1;
        }
    }
    else if (g.type == "AND")
    {
        // Fetch the fault free input and output values
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        correct_val = g.output_net.value;

        // Set the controlling value and inversion of the gate
        c = 0;
        i = 0;

        // Call the function to calculate the fault list
        calculateFaultList(g, net_list, c, inval1, inval2, correct_val, fault_lists, global_fault_list, added_to_fault);
    }
    else if (g.type == "OR")
    {
        // Fetch the fault free input and output values
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        correct_val = g.output_net.value;

        // Set the controlling value and inversion of the gate
        c = 1;
        i = 0;

        // Call the function to calculate the fault list
        calculateFaultList(g, net_list, c, inval1, inval2, correct_val, fault_lists, global_fault_list, added_to_fault);
    }
    else if (g.type == "NAND")
    {
        // Fetch the fault free input and output values
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        correct_val = g.output_net.value;

        // Set the controlling value and inversion of the gate
        c = 0;
        i = 1;

        // Call the function to calculate the fault list
        calculateFaultList(g, net_list, c, inval1, inval2, correct_val, fault_lists, global_fault_list, added_to_fault);
    }
    else if (g.type == "NOR")
    {
        // Fetch the fault free input and output values
        inval1 = net_list[g.input_nets[0].id - 1].value;
        inval2 = net_list[g.input_nets[1].id - 1].value;
        correct_val = g.output_net.value;

        // Set the controlling value and inversion of the gate
        c = 1;
        i = 1;

        // Call the function to calculate the fault list
        calculateFaultList(g, net_list, c, inval1, inval2, correct_val, fault_lists, global_fault_list, added_to_fault);
    }
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
    int file_set = 0;
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
    std::cout << "Circuit Number " << file_set + 1 << ": " << std::endl;
    std::cout << "The circuit has " << gate_list.size() << " gates." << std::endl;
    std::cout << "The circuit has " << net_list.size() << " nets." << std::endl;
    std::cout << "The circuit has " << input_list.size() << " inputs." << std::endl;
    std::cout << "The circuit has " << output_list.size() << " outputs." << std::endl;

    // Start main logic loop

    // Create the vector to store the inputs
    std::vector<int> inputs;

    // Create a global fault list
    std::vector<Fault> fault_list;

    // Variable to decide the operating mode of the program
    int mode = 1;

    if (mode == 0)
    {
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
    }
    else
    {
        // Populate the fault list with all possible faults in the circuit
        for (int i = 0; i < net_list.size(); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                int _net_id = net_list[i].id;
                int _value = j;

                Fault temp(_net_id, _value);
                fault_list.push_back(temp);
            }
        }
    }

    // Variable to store the total number of faults
    int total_faults = fault_list.size();

    // Read input from the file
    std::string in_line;
    while (std::getline(finput, in_line))
    {
        // For each character in the line
        for (char c : in_line)
        {
            if (c == '0' || c == '1')
            {
                inputs.push_back(c - '0'); // Convert char '0' or '1' to int 0 or 1
            }
        }

        // For every input in the file

        // Assign logic to input nets
        for (int i = 0; i < input_list.size(); ++i)
        {
            // Update the value of the net in the net list
            net_list[input_list[i] - 1].value = inputs[i];

            // Find all gates which have the net as an input
            for (int j = 0; j < gate_list.size(); ++j)
            {
                // For all the inputs of a given gate
                for (int k = 0; k < gate_list[j].input_nets.size(); ++k)
                {
                    // For all the input which is the current net
                    if (gate_list[j].input_nets[k].id == net_list[input_list[i] - 1].id)
                    {
                        // Update the value of the net in the gate object list
                        gate_list[j].input_nets[k].value = net_list[input_list[i] - 1].value;
                    }
                }
            }
        }

        // Create a stack for logic simulation
        std::stack<Gate> logic_stack;

        // Create a list which stores the ids of gates already added to the stack
        std::vector<int> added_to_stack;

        // Push all the initial gates onto the stack
        for (int i = 0; i < input_list.size(); ++i)
        {
            int connected_gates = net_list[input_list[i] - 1].gates_into.size();
            for (int j = 0; j < connected_gates; ++j)
            {
                // Temporary gate variable
                int index = net_list[input_list[i] - 1].gates_into[j];

                // Fetch the corresponding gate
                Gate g = gate_list[index];

                // Flag to mark if the gate has already been added to the stack
                bool isAdded = false;

                // Flag to set if the gate is assigned
                bool isAssigned = true;

                // Fetch the number of inputs for the current gate
                int ins = g.input_nets.size();

                // Check if the gate has both inputs assigned
                for (int k = 0; k < ins; ++k)
                {
                    if (g.input_nets[k].value == -1)
                    {
                        isAssigned = false;
                        break;
                    }
                }

                if (isAssigned == true)
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
        }

        // While the stack is not empty
        while (!logic_stack.empty())
        {
            // Fetch the top value from the stack
            Gate g = logic_stack.top();

            // Pop the stack
            logic_stack.pop();

            // Evaluate the output of the gate
            evaluateGate(g, net_list, gate_list);

            // Find gates with all assigned inputs not already in the stack
            for (int i = 0; i < gate_list.size(); ++i)
            {
                // Fetch the number of inputs for the current gate
                int ins = gate_list[i].input_nets.size();

                // Flag to set if the gate is assigned
                bool isAssigned = true;

                // Check if the gate has both inputs assigned
                for (int j = 0; j < ins; ++j)
                {
                    if (gate_list[i].input_nets[j].value == -1)
                    {
                        isAssigned = false;
                        break;
                    }
                }

                // Flag to mark if the gate has already been added to the stack
                bool isAdded = false;

                if (isAssigned == true)
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

        // Create a vector to store the outputs of the circuit
        std::vector<int> outputs;

        // Create a string to store the value to be printed
        std::string output_string = "";

        // Find the outputs of the circuit
        for (int i = 0; i < output_list.size(); ++i)
        {
            outputs.push_back(net_list[output_list[i] - 1].value);
            output_string += std::to_string(net_list[output_list[i] - 1].value);
        }

        // Write the binary string to the output file
        foutput << output_string << std::endl;

        // Clear the checklist for the gates added to the stack
        added_to_stack.clear();

        // Create a variable to store the fault lists
        std::vector<Fault> fault_lists[net_list.size()];

        // Create a stack for deductive fault simulation
        std::stack<Gate> fault_stack;

        // Create a list which stores the ids of gates already added to the stack
        int added_to_fault_stack[gate_list.size()];

        // Create a list which stores the ids of nets with fault lists
        int added_to_fault[net_list.size()];

        // Initially set all faults as unassigned
        for (int i = 0; i < net_list.size(); ++i)
        {
            added_to_fault[i] = 0;
        }

        // Initially set all gates as unpushed to the stack
        for (int i = 0; i < gate_list.size(); ++i)
        {
            added_to_fault_stack[i] = 0;
        }

        // For all primary inputs, initialise the fault lists to the singular values
        for (int i = 0; i < input_list.size(); ++i)
        {
            // Fetch the current value of the net, the fault stuck at value will be the inverse
            int fault_id = input_list[i];
            int fault_val = !net_list[fault_id - 1].value;

            // Create a fault object
            Fault f(fault_id, fault_val);

            // // Check for the fault net stuck at !correct_val in the global fault list
            // auto it = std::find_if(fault_list.begin(), fault_list.end(),
            //                        [fault_id, fault_val](const Fault &f)
            //                        { return f.net_id == fault_id && f.value == fault_val; });

            // // If the fault exists in the global fault list
            // if (it != fault_list.end())
            // {
            // Add the fault to the appropriate fault list
            fault_lists[fault_id - 1].push_back(f);

            // Mark the faults as added to the stack
            added_to_fault[fault_id - 1] = 1;
            // }
        }

        // Push all the initial gates onto the stack
        for (int i = 0; i < input_list.size(); ++i)
        {
            int connected_gates = net_list[input_list[i] - 1].gates_into.size();
            for (int j = 0; j < connected_gates; ++j)
            {
                // Temporary gate variable
                int index = net_list[input_list[i] - 1].gates_into[j];

                // Fetch the corresponding gate
                Gate g = gate_list[index];

                // Flag to set if the gate is assigned
                bool isAssigned = true;

                // Fetch the number of inputs for the current gate
                int ins = g.input_nets.size();

                // Check if the gate has both inputs nets assigned to the fault list
                for (int k = 0; k < ins; ++k)
                {
                    // Check if the input has been added to the fault list
                    if (added_to_fault[g.input_nets[k].id - 1] == 0)
                    {
                        isAssigned = false;
                        break;
                    }
                }

                if (isAssigned == true)
                {
                    // Flag to mark if the gate has already been added to the stack
                    bool isAdded = false;

                    // Check if the gate is not already in the fault stack
                    if (added_to_fault_stack[g.id])
                    {
                        isAdded = true;
                    }

                    // If the gate was not added previously
                    if (isAdded == false)
                    {
                        // Push the gate onto the stack
                        fault_stack.push(g);
                        added_to_fault_stack[g.id] = 1;
                    }
                }
            }
        }

        // While the fault stack is not empty
        while (!fault_stack.empty())
        {
            // Fetch the top value from the stack
            Gate g = fault_stack.top();

            // Pop the stack
            fault_stack.pop();

            // Deduce the fault list for the given gate output
            evaluateFaultList(g, net_list, gate_list, fault_lists, fault_list, added_to_fault);

            // Find gates with inputs whose faults lists have been calculated and not already pushed to the stack
            for (int i = 0; i < gate_list.size(); ++i)
            {
                // Fetch the number of inputs for the current gate
                int ins = gate_list[i].input_nets.size();

                // Flag to set if the gate is assigned
                bool isAssigned = true;

                // Check if the gate has inputs which have fault lists
                for (int j = 0; j < ins; ++j)
                {
                    // Check if the input has been added to the fault list
                    if (added_to_fault[gate_list[i].input_nets[j].id - 1] == 0)
                    {
                        isAssigned = false;
                        break;
                    }
                }

                // Flag to mark if the gate has already been added to the stack
                bool isAdded = false;

                if (isAssigned == true)
                {
                    // Check if the gate is not already in the stack
                    if (added_to_fault_stack[gate_list[i].id] == 1)
                    {
                        isAdded = true;
                    }

                    // If the gate was not added previously
                    if (isAdded == false)
                    {
                        // Push the gate onto the stack
                        fault_stack.push(gate_list[i]);
                        added_to_fault_stack[gate_list[i].id] = 1;
                    }
                }
            }
        }

        // Create a list to store the faults detected at the output
        std::vector<Fault> detected_faults;

        // For all the nets in the output lists, create a union of all the faults
        for (int i = 0; i < output_list.size(); ++i)
        {
            // Make a copy of the net fault list
            std::vector<Fault> list1 = fault_lists[output_list[i] - 1];

            // Make a temporary result variable
            std::vector<Fault> result;

            // Union the list with the fault list of the output line
            std::set_union(list1.begin(), list1.end(), detected_faults.begin(), detected_faults.end(), std::back_inserter(result));

            // Copy the temporary result back into the main detected fault list
            detected_faults = result;
        }

        // Sort the detected fault list
        std::sort(detected_faults.begin(), detected_faults.end());

        std::cout << "Detected Faults Size: " << detected_faults.size() << std::endl;

        // Open a file for printing the faults detected in the simulation
        std::ofstream outputFile("d_" + filename, std::ios::app);

        // Write the size of the vector on the first line
        outputFile << detected_faults.size() << std::endl;

        // Write the elements of the vector on separate lines
        for (const Fault &fault : detected_faults)
        {
            outputFile << fault.net_id << " " << fault.value << std::endl;
        }

        outputFile << "\n"
                   << std::endl;

        // Close the file
        outputFile.close();
    }

    // Clear the gate and net lists
    gate_list.clear();
    net_list.clear();

    // Clear the input and output lists
    input_list.clear();
    output_list.clear();

    // Clear the variables related to deductive simulation

    // Close the files
    fin.close();
    finput.close();
    foutput.close();

    return 0;
}