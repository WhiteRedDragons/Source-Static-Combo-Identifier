//==========================================================================//
// ShiroDkxtro2 Software - Outputs the values for Statics in a Missing Combo
//==========================================================================//
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <bitset>	// bit manipulation
#include <regex>	// Cursed regex stuff
#include <iomanip>

using namespace System;
using namespace std;

struct ShaderStatic_t
{
	string sName;
	unsigned int nMinValue;
	unsigned int nMaxValue;
	unsigned int nFinalValue = 0; // Technically not needed

	// This was mainly used for debugging
	// I added nFinalValue later but it isn't technically needed so not implemented here
	ShaderStatic_t() : sName(""), nMinValue(0), nMaxValue(1) {}
	ShaderStatic_t(const string& name, unsigned int minVal, unsigned int maxVal)
		: sName(name), nMinValue(minVal), nMaxValue(maxVal) {}
};

// We shall allocate all statics on this vector
vector<ShaderStatic_t> pStatics;

// Mainly used for debugging the bitmask
void PrintBits(unsigned int value, int bitCount)
{
	bitset<32> bits(value);
	cout << bits.to_string().substr(32 - bitCount);
}

// Alternative way of calculating this would be bitshifting it around and adding a counter when & 0x1
unsigned int BitsRequired(unsigned int value)
{
	return static_cast<unsigned int>(ceil(log2(value + 1)));
}

void CalculateMissingCombo(unsigned int missingCombo)
{
	vector<unsigned int> values(pStatics.size());
	unsigned int combo = missingCombo;

	cout << "Combo number: " << combo << endl;
//	PrintBits(combo, 12); // Display the last 12 bits

	// FIXME: This whole thing doesn't account for minimum values of a combo
	// Honestly you shouldn't be using those anyways.. but that should probably be supported
	vector<unsigned int> BitMasks(pStatics.size());
	vector<unsigned int> PowerOfTwos(pStatics.size());
	unsigned int nPowerOfTwo = 1;
	for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
	{
		PowerOfTwos[nStatic] = nPowerOfTwo;
		BitMasks[nStatic] = (pStatics[nStatic].nMaxValue) * nPowerOfTwo;
		for (unsigned int n = 0; n < BitsRequired(pStatics[nStatic].nMaxValue); n++)
		{
			nPowerOfTwo *= 2;
		}
	}

	// We will use the next loop to figure out which one is the largest string
	// This is just used for text alignment
	unsigned int nLargestString = 1;

	// Now do it again but reverse the action
	for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
	{
		// Mask away only the region we are interested in
		unsigned int nIsolated = combo & BitMasks[nStatic];

		// Divide by its power of two to bring back the original range
		nIsolated /= PowerOfTwos[nStatic];

		// Store the value
		values[nStatic] = nIsolated;

		// If the name of this combo is longer than the others set it as the new maximum length
		unsigned int nTextLength = pStatics[nStatic].sName.length();
		nLargestString = nTextLength > nLargestString ? nTextLength : nLargestString;
	}

	// After the name comes a ", then a spacebar
	// So adjust for those
	nLargestString += 2;

	for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
	{
		pStatics[nStatic].nFinalValue = values[nStatic];
		cout << "\"" << pStatics[nStatic].sName << "\"";
		// Adjust to the right. Account for the name length variance
		cout << right << setw(nLargestString - pStatics[nStatic].sName.length() + 2);
		cout << "\"" << pStatics[nStatic].nFinalValue << "\"" << endl;
	}
}

unsigned int CalculateTotalPermutations()
{
	//...You gotta have one or there is not much to render is there
	unsigned int totalPermutations = 1;

	// This should be the easiest approach to the permutation problem
	for (const auto& shaderStatic : pStatics)
	{
		unsigned int numValues = shaderStatic.nMaxValue - shaderStatic.nMinValue + 1;
		totalPermutations *= numValues;
	}
	return totalPermutations;
}

//==========================================================================//
// Main function
//==========================================================================//
int main(array<System::String ^> ^args)
{
	cout << "Enter the path\\name.fxc of the Shader" << endl;
	
	// Using string for this ( -- I love c++'s string  <3 --)
	string sUserInput;
	getline(cin, sUserInput);

	// Load the file from the disk, yes this function hates strings so use cc
	ifstream FXC_File(sUserInput.c_str(), std::ios::binary);

	// Make sure its open.
	if (FXC_File.is_open())
	{
		// We could stop after a couple lines but lets just go through the whole file
		// Makes my life easier
		string line;

		// I accidentally summoned some C++ eldritch horror
		// But it does my bidding, henceforth I am its Master
		regex staticRegex(R"~(// STATIC:\s*\"([^\"]+)\"\s*\"(\d+\.\.\d+)\")~");
		smatch match;
		while (getline(FXC_File, line))
		{
			// If we find a match,
			if (regex_search(line, match, staticRegex))
			{
				// And we are in the range
				if (match.size() == 3)
				{
					string name = match[1].str();
					string range = match[2].str();

					// Find the .. in "minimum .. maximum" of a static combo
					// That way we can skip it for the maximum value
					size_t dotdot = range.find("..");
					if (dotdot != string::npos)
					{
						// I use stoi isntead of atoi here since atoi wants a cc not a string
						unsigned int minVal = stoi(range.substr(0, dotdot));
						unsigned int maxVal = stoi(range.substr(dotdot + 2));

						// Create a new ShaderStatic,
						// init it with the values we extracted and attach it to the tail of the vector
						ShaderStatic_t shaderStatic(name, minVal, maxVal);
						pStatics.push_back(shaderStatic);
					}
				}
			}
		}
		FXC_File.close(); // We no longer need the file so close it

		cout << endl << "Number of \"// STATIC:\"'s found is " << pStatics.size() << endl;
		cout << "Maximum possible number of permutations : " << CalculateTotalPermutations() << endl;
		cout << "Enter the number of the combo that appears to be missing.." << endl;
		getline(cin, sUserInput);

		// Convert via stoi we need a number not a string
		unsigned int nMissingCombo = stoi(sUserInput);

		cout << "Value : " << nMissingCombo << " Binary : ";
		PrintBits(nMissingCombo, 10); cout << endl;

		// Combo number goes back to front
		// This function will spew all the information desired
		CalculateMissingCombo(nMissingCombo);

		// Make sure if we feed this back in we get the same number back
		cout << endl << "Consistency check.." << endl;
		unsigned int nResultCombo = 0;
		unsigned int nPowerOfTwo = 1;
		for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
		{
			nResultCombo += pStatics[nStatic].nFinalValue * nPowerOfTwo;
			for (unsigned int n = 0; n < BitsRequired(pStatics[nStatic].nMaxValue); n++)
			{
				nPowerOfTwo *= 2;
			}
		}

		cout << "Binary : "; PrintBits(nResultCombo, 10); cout << endl;
		cout << "Integer : " << nResultCombo << endl;

		if (nResultCombo != nMissingCombo)
		{
			cout << "Something went wrong. Make sure your input is smaller than the maximum number of combos on this Shader.." << endl;
			cout << "Please note: This result is calculated based on the system used to construct the number." << endl;
			cout << "You can see how its calculated near the end of the include file for the Shader" << endl;
			cout << "Because this file changes with each shader compile," << endl;
			cout << "you should recompile the dll before you fetch a missing combo number!" << endl;
		}
		else
			cout << "Consistent result. You should double check if this makes sense regardless! :)" << endl;
		
		cout << endl << "Press Enter to close the program" << endl;
		getline(cin, sUserInput); // Sleep instead?
	}
	else
	{
		cout << "Could not open file for whatever reason : " << sUserInput << endl;
	}

	// Not technically required for a c++ main function but just return 0 anyways
    return 0;
}
