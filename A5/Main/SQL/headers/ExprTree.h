
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include "MyDB_Table.h"
#include <string>
#include <vector>
#include <utility>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

enum class ReturnType { STRING, INT, DOUBLE, BOOL, ERROR };

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

public:
	virtual ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess);
	virtual string toString () = 0;
	virtual ~ExprTree () {}
};



inline bool isNumeric(ReturnType returnType) {
	return (returnType == ReturnType::INT || returnType == ReturnType::DOUBLE);
}

inline bool bothNumeric(ReturnType leftType, ReturnType rightType) {
	return isNumeric(leftType) && isNumeric(rightType);
}

// includes all arithmetic except +
inline ReturnType typeCheckForArithmetic(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, ExprTreePtr lhs, ExprTreePtr rhs) {
	// type check the left and right sides
	ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
	ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
	
	// return an error if the left and right side of this arithmetic op are not valid
	if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
		return ReturnType::ERROR;
	}

	// the left and right side must be an int or a double
	if (!bothNumeric(leftType, rightType)) {
		return ReturnType::ERROR;
	}

	// if they're both an int, return an int
	if (leftType == ReturnType::INT && rightType == ReturnType::INT) {
		return ReturnType::INT;
	}

	return ReturnType::DOUBLE;
}

inline ReturnType typeCheckForComparisons(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, ExprTreePtr lhs, ExprTreePtr rhs) {
		// type check the left and right sides
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
		
		// return an error if the left and right side of the comparison are not valid
		if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
			return ReturnType::ERROR;
		}

		// if one is a string, both must be a string
		if ((leftType == ReturnType::STRING && rightType != ReturnType::STRING) ||
		    (leftType != ReturnType::STRING && rightType == ReturnType::STRING)) {
			return ReturnType::ERROR;
		}

		// if one is numeric, both must be numeric
		if ((isNumeric(leftType) && !isNumeric(rightType)) ||
		    (!isNumeric(leftType) && isNumeric(rightType))) {
			return ReturnType::ERROR;
		}

		return ReturnType::BOOL;
}

// for checking equality expressions. Ex: a == b, a != b, 
inline ReturnType typeCheckForEqualities(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, ExprTreePtr lhs, ExprTreePtr rhs) {
		// type check the left and right sides
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
		
		// return an error if the left and right side of the equality are not valid
		if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
			return ReturnType::ERROR;
		}

		// if one is a string, both must be a string
		if ((leftType == ReturnType::STRING && rightType != ReturnType::STRING) ||
		    (leftType != ReturnType::STRING && rightType == ReturnType::STRING)) {
			return ReturnType::ERROR;
		}

		// if one is numeric, both must be numeric
		if ((isNumeric(leftType) && !isNumeric(rightType)) ||
		    (!isNumeric(leftType) && isNumeric(rightType))) {
			return ReturnType::ERROR;
		}

		// if one is a bool, both must be bool
		if ((leftType == ReturnType::BOOL && rightType != ReturnType::BOOL) ||
		    (leftType != ReturnType::BOOL && rightType == ReturnType::BOOL)) {
			return ReturnType::ERROR;
		}


		return ReturnType::BOOL;
}

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return ReturnType::BOOL;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}	
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return ReturnType::DOUBLE;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	

	~DoubleLiteral () {}
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return ReturnType::INT;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return ReturnType::STRING;
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	~StringLiteral () {}
};

class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
	}

	ReturnType typeCheck(map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		string name;
		bool found = false;
		for (pair<string, string> currPair : tablesToProcess) {
			if (currPair.second == this->tableName) {
				name = currPair.first;
				found = true;
				break;
			}
		}

		if (!found) {
			cout << "Invalid alias " <<  this->tableName << endl;
			return ReturnType::ERROR;
		}

		// We should have already caught that tableName is in allTables
		MyDB_TablePtr table = allTables.at(name);
		MyDB_SchemaPtr schema = table->getSchema();
		pair<int, MyDB_AttTypePtr> attPair = schema->getAttByName(this->attName);

		// No need to print an error since schema->getAttByName() will already
		// print something if there's an error.
		if (attPair.first == -1)
			return ReturnType::ERROR;

		MyDB_AttTypePtr att = attPair.second;

		if (att->isBool())
			return ReturnType::BOOL;
		
		if (att->promotableToInt())
			return ReturnType::INT;

		if (att->promotableToDouble())
			return ReturnType::DOUBLE;

		return ReturnType::STRING;
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	

	~Identifier () {}
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForArithmetic(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~MinusOp () {}
};

class PlusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {

			// type check the left and right sides
			ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
			ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
			
			// return an error if the left and right side of the plus op are not valid
			if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
				return ReturnType::ERROR;
			}

			// if either side is a string, then the type is string
			if (leftType == ReturnType::STRING || rightType == ReturnType::STRING) {
				return ReturnType::STRING;
			}

			// if it's not a string, then left and right side must be numeric
			if (!bothNumeric(leftType, rightType)) {
				return ReturnType::ERROR;
			}

			// if they're both an int, return an int
			if (leftType == ReturnType::INT && rightType == ReturnType::INT) {
				return ReturnType::INT;
			}

			return ReturnType::DOUBLE;
		}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForArithmetic(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForArithmetic(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForComparisons(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForComparisons(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}
	
	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForEqualities(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		// type check the left and right sides
		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
		
		// return an error if the left and right side of the OR op are not valid
		if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
			return ReturnType::ERROR;
		}

		// both sides must be BOOL
		if (leftType != ReturnType::BOOL || rightType != ReturnType::BOOL) {
			return ReturnType::ERROR;
		}

		return ReturnType::BOOL;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) {
		return typeCheckForEqualities(allTables, tablesToProcess, lhs, rhs);
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	ReturnType typeCheck(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess) { 
		ReturnType childType = child->typeCheck(allTables, tablesToProcess);
		if (childType != ReturnType::BOOL) {
			return ReturnType::ERROR;
		}
		return ReturnType::BOOL;
	}


	string toString () {
		return "!(" + child->toString () + ")";
	}	

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}	

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}	

	~AvgOp () {}
};

// inline bool isNumeric(ReturnType returnType) {
// 	return (returnType == ReturnType::INT || returnType == ReturnType::DOUBLE);
// }

// inline bool bothNumeric(ReturnType leftType, ReturnType rightType) {
// 	return isNumeric(leftType) && isNumeric(rightType);
// }

// inline ReturnType typeCheckForComparisons(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, ExprTreePtr lhs, ExprTreePtr rhs) {
// 		// type check the left and right sides
// 		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
// 		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
		
// 		// return an error if the left and right side of the comparison are not valid
// 		if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
// 			return ReturnType::ERROR;
// 		}

// 		// if one is a string, both must be a string
// 		if ((leftType == ReturnType::STRING && rightType != ReturnType::STRING) ||
// 		    (leftType != ReturnType::STRING && rightType == ReturnType::STRING)) {
// 			return ReturnType::ERROR;
// 		}

// 		// if one is numeric, both must be numeric
// 		if ((isNumeric(leftType) && !isNumeric(rightType)) ||
// 		    (!isNumeric(leftType) && isNumeric(rightType))) {
// 			return ReturnType::ERROR;
// 		}

// 		return ReturnType::BOOL;
// }

// // for checking equality expressions. Ex: a == b, a != b, 
// inline ReturnType typeCheckForEqualities(unordered_map<string, MyDB_TablePtr> &allTables, vector<pair<string, string>> &tablesToProcess, ExprTreePtr lhs, ExprTreePtr rhs) {
// 		// type check the left and right sides
// 		ReturnType leftType = lhs->typeCheck(allTables, tablesToProcess);
// 		ReturnType rightType = rhs->typeCheck(allTables, tablesToProcess);
		
// 		// return an error if the left and right side of the equality are not valid
// 		if (leftType == ReturnType::ERROR || rightType == ReturnType::ERROR) {
// 			return ReturnType::ERROR;
// 		}

// 		// if one is a string, both must be a string
// 		if ((leftType == ReturnType::STRING && rightType != ReturnType::STRING) ||
// 		    (leftType != ReturnType::STRING && rightType == ReturnType::STRING)) {
// 			return ReturnType::ERROR;
// 		}

// 		// if one is numeric, both must be numeric
// 		if ((isNumeric(leftType) && !isNumeric(rightType)) ||
// 		    (!isNumeric(leftType) && isNumeric(rightType))) {
// 			return ReturnType::ERROR;
// 		}

// 		// if one is a bool, both must be bool
// 		if ((leftType == ReturnType::BOOL && rightType != ReturnType::BOOL) ||
// 		    (leftType != ReturnType::BOOL && rightType == ReturnType::BOOL)) {
// 			return ReturnType::ERROR;
// 		}


// 		return ReturnType::BOOL;
// }

#endif
