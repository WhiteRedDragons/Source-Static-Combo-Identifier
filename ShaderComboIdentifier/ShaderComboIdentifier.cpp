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
	unsigned int combo = missingCombo;
	cout << "Combo number: " << combo << endl;

	unsigned int nLargestString = 1;
	unsigned int nOffset = 1;

	// We first need to compute the total offset for the largest multiplier
	for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
	{
		nOffset *= pStatics[nStatic].nMaxValue + 1;
	}

	// Adjust the multiplier
	// FIXME: Figure out why EXACTLY this happens
	// I assume this happens because both the first and second static
	// would be forced to share the same bit when the first static needs more than one bit
	// example, 1 + 2 is no problem you get 0000 0011 but when you do 2 + 2 you get 0000 0100
	// when the second combo could be a maximum of 2 you'd then run into the issue that two combos need the same offset
	if (pStatics[0].nMaxValue == 1)
		nOffset /= 2;

	// Loop through the statics in reverse order
	// Important its int, apparently Vector doesn't like uint
	for (int nStatic = pStatics.size() - 1; nStatic >= 0; --nStatic)
	{
		// Calculate the current offset for this static
		unsigned int nCurrentOffset = pStatics[nStatic].nMaxValue + 1;

		// Convert combo to float and divide by the current offset
		float floatCombo = static_cast<float>(combo);
		float floatValue = floatCombo / (float)nOffset;

//		cout << floatCombo << " / " << nOffset << " = " << floatValue << endl;

		// Round down and convert to integer to get the final value
		pStatics[nStatic].nFinalValue = static_cast<unsigned int>(std::floor(floatValue));

		// Subtract value * offset from combo
		combo -= pStatics[nStatic].nFinalValue * nOffset;

		// Update nOffset for the next static
		// We have to do this based on the value of that *next* static in the line
		int nLookup = (nStatic - 1) < 0 ? 0 : nStatic - 1;
		int nDivider = pStatics[nLookup].nMaxValue + 1;

		nOffset /= nDivider;

		// Determine the largest string for formatting
		unsigned int nTextLength = pStatics[nStatic].sName.length();
		nLargestString = nTextLength > nLargestString ? nTextLength : nLargestString;
	}

	// Adjust for name formatting
	nLargestString += 2;

	// Output the results
	for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
	{
		cout << "\"" << pStatics[nStatic].sName << "\"";
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

		// I don't know why this is the case but its what I see happening in the includes..
		unsigned int nMultiplier = 1;
		unsigned int nResultCombo = 0;

		// We first need to compute the total offset for the largest multiplier
		for (unsigned int nStatic = 0; nStatic < pStatics.size(); nStatic++)
		{
			nMultiplier *= pStatics[nStatic].nMaxValue + 1;
		}

		// FIXME: Figure out why this happens
		// This is merely an observation from Includes
		// If the first combo is not a max value of 1, it will do *2 instead of *1
		// I assume its to force an extra bit of space
		if (pStatics[0].nMaxValue == 1)
			nMultiplier /= 2;

		// Important its int, apparently Vector doesn't like uint
		for (int nStatic = (pStatics.size() - 1); nStatic >= 0; --nStatic)
		{
			nResultCombo += pStatics[nStatic].nFinalValue * nMultiplier;

			// Adjust the multiplier
			// FIXME: Figure out why EXACTLY this happens
			// I assume this happens because both the first and second static
			// would be forced to share the same bit when the first static needs more than one bit
			// example, 1 + 2 is no problem you get 0000 0011 but when you do 2 + 2 you get 0000 0100
			// when the second combo could be a maximum of 2 you'd then run into the issue that two combos need the same offset
			if (nStatic == 0 && pStatics[0].nMaxValue == 1)
				nMultiplier = 1;
			else if (nStatic == 0)
				nMultiplier = 2;
			else
				nMultiplier /= (pStatics[nStatic - 1].nMaxValue + 1);
		}

		cout << "Binary : "; PrintBits(nResultCombo, 10); cout << endl;
		cout << "Integer : " << nResultCombo << endl;

		if (nResultCombo != nMissingCombo)
		{
			// Oddball scenario
			// Because the first value is multiplied by *2 instead of *1
			// You cannot have uneven combo numbers, since the first bit will always be a 0
			// In fact, throw the result value through the logic and you get the same static values
			// So the result is correct, its just not the same as the original
			if (pStatics[0].nMaxValue > 1 && nResultCombo == (nMissingCombo - 1))
			{
				cout << "Correct result, sort of. You should double check if this makes sense regardless! :)" << endl;
			}
			else
			{
				cout << "Something went wrong. Make sure your input is smaller than the maximum number of combos on this Shader.." << endl;
				cout << "NOTE 1: This Value is calculated based on the system used to construct the number in an include." << endl;
				cout << "NOTE 2: However this is prone to mistakes as its done based on observation." << endl;
				cout << "You can see how its calculated near the end of the include file for the Shader" << endl;
				cout << "Because this file changes with each shader compile," << endl;
				cout << "you should recompile the dll before you fetch a missing combo number!" << endl;
			}
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
