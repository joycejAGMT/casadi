/*
 *    This file is part of CasADi.
 *
 *    CasADi -- A symbolic framework for dynamic optimization.
 *    Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
 *
 *    CasADi is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    CasADi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with CasADi; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "sx.hpp"
#include "../matrix/matrix.hpp"
#include <stack>
#include <cassert>
#include "../casadi_math.hpp"

using namespace std;
namespace CasADi{

// Allocate storage for the caching
#ifdef CACHING_CONSTANTS
std::unordered_map<int,IntegerSXNode*> IntegerSXNode::cached_constants_;
std::unordered_map<double,RealtypeSXNode*> RealtypeSXNode::cached_constants_;
#endif // CASHING_CONSTANTS

SX::SX(){
  node = casadi_limits<SX>::nan.node;
  node->count++;
}

SX::SX(SXNode* node_, bool dummy) : node(node_){
  node->count++;
}

SX SX::create(SXNode* node){
  return SX(node,false);
}

SX::SX(const SX& scalar){
  node = scalar.node;
  node->count++;
}

SX::SX(double val){
  int intval = int(val);
  if(val-intval == 0){ // check if integer
    if(intval == 0)             node = casadi_limits<SX>::zero.node;
    else if(intval == 1)        node = casadi_limits<SX>::one.node;
    else if(intval == 2)        node = casadi_limits<SX>::two.node;
    else if(intval == -1)       node = casadi_limits<SX>::minus_one.node;
    else                        node = IntegerSXNode::create(intval);
    node->count++;
  } else {
    if(isnan(val))              node = casadi_limits<SX>::nan.node;
    else if(isinf(val))         node = val > 0 ? casadi_limits<SX>::inf.node : casadi_limits<SX>::minus_inf.node;
    else                        node = RealtypeSXNode::create(val);
    node->count++;
  }
}

SX::SX(const std::string& name){
  node = new SymbolicSXNode(name);  
  node->count++;
}

SX::~SX(){
  if(--node->count == 0) delete node;
}

SX& SX::operator=(const SX &scalar){
  // quick return if the old and new pointers point to the same object
  if(node == scalar.node) return *this;

  // decrease the counter and delete if this was the last pointer	
  if(--node->count == 0) delete node;

  // save the new pointer
  node = scalar.node;
  node->count++;
  return *this;
}

SXNode* SX::assignNoDelete(const SX& scalar){
  // Return value
  SXNode* ret = node;

  // quick return if the old and new pointers point to the same object
  if(node == scalar.node) return ret;

  // decrease the counter but do not delete if this was the last pointer
  --node->count;

  // save the new pointer
  node = scalar.node;
  node->count++;
  
  // Return a pointer to the old node
  return ret;
}

SX& SX::operator=(double scalar){
  return *this = SX(scalar);
}

std::ostream &operator<<(std::ostream &stream, const SX &scalar)
{
  scalar.node->print(stream);  
  return stream;
}

void SX::print(std::ostream &stream, long& remaining_calls) const{
  if(remaining_calls>0){
    remaining_calls--;
    node->print(stream,remaining_calls);
  } else {
    stream << "...";
  }
}

SX& operator+=(SX &ex, const SX &el){
  return ex = ex + el;
}

SX& operator-=(SX &ex, const SX &el){
  return ex = ex - el;
}

SX SX::operator-() const{
  if(node->hasDep() && node->getOp() == NEG)
    return node->dep(0);
  else if(node->isZero())
    return 0;
  else if(node->isMinusOne())
    return 1;
  else if(node->isOne())
    return -1;
  else
   return BinarySXNode::createT<NEG>( *this);
}

SX& operator*=(SX &ex, const SX &el){
 return ex = ex * el;
}

SX& operator/=(SX &ex, const SX &el){
  return ex = ex / el;
}

SX SX::sign() const{
  if(isConstant())
    return CasADi::sign(getValue());
  else
    return BinarySXNode::createT<SIGN>( *this);
}

SX SX::erfinv() const{
  return BinarySXNode::createT<ERFINV>( *this);
}

SX SX::add(const SX& y) const{
  // NOTE: Only simplifications that do not result in extra nodes area allowed
    
  if(node->isZero())
    return y;
  else if(y->isZero()) // term2 is zero
    return *this;
  else if(y.isBinary() && y.getOp()==NEG) // x + (-y) -> x - y
    return sub(-y);
  else if(isBinary() && getOp()==NEG) // (-x) + y -> y - x
    return y.sub(getDep());
  else if(isBinary() && getOp()==MUL && 
          y.isBinary() && y.getOp()==MUL && 
          getDep(0).isConstant() && getDep(0).getValue()==0.5 && 
          y.getDep(0).isConstant() && y.getDep(0).getValue()==0.5 &&
          y.getDep(1).isEquivalent(getDep(1))) // 0.5x+0.5x = x
    return getDep(1);
  else if(isBinary() && getOp()==DIV && 
          y.isBinary() && y.getOp()==DIV && 
          getDep(1).isConstant() && getDep(1).getValue()==2 && 
          y.getDep(1).isConstant() && y.getDep(1).getValue()==2 &&
          y.getDep(0).isEquivalent(getDep(0))) // x/2+x/2 = x
    return getDep(0);
    
  else // create a new branch
    return BinarySXNode::createT<ADD>( *this, y);
}

SX SX::sub(const SX& y) const{
  // Only simplifications that do not result in extra nodes area allowed
    
  if(y->isZero()) // term2 is zero
    return *this;
  if(node->isZero()) // term1 is zero
    return -y;
  if(isEquivalent(y)) // the terms are equal
    return 0;
  else if(y.isBinary() && y.getOp()==NEG) // x - (-y) -> x + y
    return add(-y);
  else // create a new branch
    return BinarySXNode::createT<SUB>( *this, y);
}

SX SX::mul(const SX& y) const{
  // Only simplifications that do not result in extra nodes area allowed
  if(!isConstant() && y.isConstant())
    return y.mul(*this);
  else if(node->isZero() || y->isZero()) // one of the terms is zero
    return 0;
  else if(node->isOne()) // term1 is one
    return y;
  else if(y->isOne()) // term2 is one
    return *this;
  else if(y->isMinusOne())
    return -(*this);
  else if(node->isMinusOne())
    return -y;
  else if(y.isBinary() && y.getOp()==INV)
    return (*this)/y.inv();
  else if(isBinary() && getOp()==INV)
    return y/inv();
  else if(isConstant() && y.isBinary() && y.getOp()==MUL && y.getDep(0).isConstant() && getValue()*y.getDep(0).getValue()==1) // 5*(0.2*x) = x
    return y.getDep(1);
  else if(isConstant() && y.isBinary() && y.getOp()==DIV && y.getDep(1).isConstant() && getValue()==y.getDep(1).getValue()) // 5*(x/5) = x
    return y.getDep(0);
  else if(isBinary() && getOp()==DIV && getDep(1).isEquivalent(y)) // ((2/x)*x)
    return getDep(0);
  else if(y.isBinary() && y.getOp()==DIV && y.getDep(1).isEquivalent(*this)) // ((2/x)*x)
    return y.getDep(0);
  else     // create a new branch
    return BinarySXNode::createT<MUL>(*this,y);
}

bool SX::isDoubled() const{
  return isOp(ADD) && node->dep(0).isEquivalent(node->dep(1));
}
    
bool SX::isSquared() const{
  return isOp(MUL) && node->dep(0).isEquivalent(node->dep(1));
}

bool SX::isEquivalent(const SX&y, int depth) const{
  if (isEqual(y)) return true;
  if (isConstant() && y.isConstant() && y.getValue()==getValue());
  if (depth==0) return false;
  
  if (isBinary() && y.isBinary() && getOp()==y.getOp()) {
    if (getDep(0).isEquivalent(y.getDep(0),depth-1)  && getDep(1).isEquivalent(y.getDep(1),depth-1)) return true;
    return (operation_checker<CommChecker>(getOp()) && getDep(0).isEquivalent(y.getDep(1),depth-1)  && getDep(1).isEquivalent(y.getDep(0),depth-1));
  }
  return false;
}

SX SX::div(const SX& y) const{
  // Only simplifications that do not result in extra nodes area allowed

  if(y->isZero()) // term2 is zero
    return casadi_limits<SX>::nan;
  else if(node->isZero()) // term1 is zero
    return 0;
  else if(y->isOne()) // term2 is one
    return *this;
  else if(isEquivalent(y)) // terms are equal
    return 1;
  else if(isDoubled() && y.isEqual(2))
    return node->dep(0);
  else if(isOp(MUL) && y.isEquivalent(node->dep(0)))
    return node->dep(1);
  else if(isOp(MUL) && y.isEquivalent(node->dep(1)))
    return node->dep(0);
  else if(node->isOne())
    return y.inv();
  else if(y.isBinary() && y.getOp()==INV)
    return (*this)*y.inv();
  else if(isDoubled() && y.isDoubled())
    return node->dep(0) / y->dep(0);
  else if(y.isConstant() && isBinary() && getOp()==DIV && getDep(1).isConstant() && y.getValue()*getDep(1).getValue()==1) // (x/5)/0.2 
    return getDep(0);
  else if(y.isBinary() && y.getOp()==MUL && y.getDep(1).isEquivalent(*this)) // x/(2*x) = 1/2
    return BinarySXNode::createT<DIV>(1,y.getDep(0));
  else if(isBinary() && getOp()==NEG && getDep(0).isEquivalent(y))      // (-x)/x = -1
    return -1;
  else if(y.isBinary() && y.getOp()==NEG && y.getDep(0).isEquivalent(*this))      // x/(-x) = 1
    return -1;
  else if(y.isBinary() && y.getOp()==NEG && isBinary() && getOp()==NEG && getDep(0).isEquivalent(y.getDep(0)))      // (-x)/(-x) = 1
    return 1;
  else // create a new branch
    return BinarySXNode::createT<DIV>(*this,y);
}

SX SX::inv() const{
  if(node->hasDep() && node->getOp()==INV){
    return node->dep(0);
  } else {
    return BinarySXNode::createT<INV>(*this);
  }
}

Matrix<SX> SX::add(const Matrix<SX>& y) const {
 return Matrix<SX>(*this)+y;
}
Matrix<SX> SX::sub(const Matrix<SX>& y) const {
 return Matrix<SX>(*this)-y;
}
Matrix<SX> SX::mul(const Matrix<SX>& y) const {
 return Matrix<SX>(*this)*y;
}
Matrix<SX> SX::div(const Matrix<SX>& y) const { 
  return Matrix<SX>(*this)/y;
}
Matrix<SX> SX::fmin(const Matrix<SX>& b) const { 
  return Matrix<SX>(*this).fmin(b);
}
Matrix<SX> SX::fmax(const Matrix<SX>& b) const {
  return Matrix<SX>(*this).fmax(b);
}
Matrix<SX> SX::constpow(const Matrix<SX>& n) const {
 return Matrix<SX>(*this).__constpow__(n);
}

SX operator+(const SX &x, const SX &y){
  return x.add(y);
}

SX operator-(const SX &x, const SX &y){
  return x.sub(y);
}

SX operator*(const SX &x, const SX &y){
  return x.mul(y);
}


SX operator/(const SX &x, const SX &y){
  return x.div(y);
}

SX operator<=(const SX &a, const SX &b){
  return b>=a;
}

SX operator>=(const SX &a, const SX &b){
  // Move everything to one side
  SX x = a-b;
  if(x.isSquared() || x.isOp(FABS))
    return 1;
  else if(x->isConstant())
    return x->getValue()>=0; // ok since the result will be either 0 or 1, i.e. no new nodes
  else
    return BinarySXNode::createT<STEP>(x);
}

SX operator<(const SX &a, const SX &b){
  return !(a>=b);
}

SX operator>(const SX &a, const SX &b){
  return !(a<=b);
}

SX operator&&(const SX &a, const SX &b){
  return a+b>=2;
}

SX operator||(const SX &a, const SX &b){
  return !(!a && !b);
}

SX operator==(const SX &x, const SX &y){
  if(x.isEqual(y))
    return 1; // this also covers the case when both are constant and equal
  else if(x.isConstant() && y.isConstant())
    return 0;
  else // create a new node
    return BinarySXNode::createT<EQUALITY>(x,y);
}

SX operator!=(const SX &a, const SX &b){
  return !(a == b);
}

SX operator!(const SX &a){
  return 1-a;
}

SXNode* const SX::get() const{
  return node;
}

const SXNode* SX::operator->() const{
  return node;
}

SXNode* SX::operator->(){
  return node;
}

SX if_else(const SX& cond, const SX& if_true, const SX& if_false){
  return if_false + (if_true-if_false)*cond;
}

SX SX::binary(int op, const SX& x, const SX& y){
  return BinarySXNode::create(Operation(op),x,y);    
}

SX SX::unary(int op, const SX& x){
  return BinarySXNode::create(Operation(op),x);  
}

// SX::operator vector<SX>() const{
//   vector<SX> ret(1);
//   ret[0] = *this;
//   return ret;
// }

string SX::toString() const{
  stringstream ss;
  ss << *this;
  return ss.str();
}

bool SX::isLeaf() const {
  if (!node) return true;
  return node->isConstant() || node->isSymbolic();
}

bool SX::isCommutative() const{
  if (!isBinary()) throw CasadiException("SX::isCommutative: must be binary");
  return operation_checker<CommChecker>(getOp());
}

bool SX::isConstant() const{
  return node->isConstant();
}

bool SX::isInteger() const{
  return node->isInteger();
}

bool SX::isSymbolic() const{
  return node->isSymbolic();
}

bool SX::isBinary() const{
  return node->hasDep();
}

bool SX::isZero() const{
  return node->isZero();
}

bool SX::isOne() const{
  return node->isOne();
}

bool SX::isMinusOne() const{
  return node->isMinusOne();
}

bool SX::isNan() const{
  return node->isNan();
}

bool SX::isInf() const{
  return node->isInf();
}

bool SX::isMinusInf() const{
  return node->isMinusInf();
}

const std::string& SX::getName() const{
  return node->getName();
}

int SX::getOp() const{
  return node->getOp();
}

bool SX::isOp(int op) const{
  return isBinary() && op==getOp();
}

bool SX::isEqual(const SX& scalar) const{
  return node->isEqual(scalar);
}

double SX::getValue() const{
  return node->getValue();
}

int SX::getIntValue() const{
  return node->getIntValue();
}

SX SX::getDep(int ch) const{
  casadi_assert(ch==0 || ch==1;)
  return node->dep(ch);
}

int SX::getNdeps() const {
  if (!isBinary()) throw CasadiException("SX::getNdeps: must be binary");
  return casadi_math<double>::ndeps(getOp());
}

long SX::__hash__() const {
   if (!node) return 0;
   return (long) node;
}

const SX casadi_limits<SX>::zero(new ZeroSXNode(),false); // node corresponding to a constant 0
const SX casadi_limits<SX>::one(new OneSXNode(),false); // node corresponding to a constant 1
const SX casadi_limits<SX>::two(IntegerSXNode::create(2),false); // node corresponding to a constant 2
const SX casadi_limits<SX>::minus_one(new MinusOneSXNode(),false); // node corresponding to a constant -1
const SX casadi_limits<SX>::nan(new NanSXNode(),false);
const SX casadi_limits<SX>::inf(new InfSXNode(),false);
const SX casadi_limits<SX>::minus_inf(new MinusInfSXNode(),false);

bool casadi_limits<SX>::isZero(const SX& val){ 
  return val.isZero();
}

bool casadi_limits<SX>::isOne(const SX& val){ 
  return val.isOne();
}

bool casadi_limits<SX>::isMinusOne(const SX& val){ 
  return val.isMinusOne();
}

bool casadi_limits<SX>::isConstant(const SX& val){
  return val.isConstant();
}

bool casadi_limits<SX>::isInteger(const SX& val){
  return val.isInteger();
}

bool casadi_limits<SX>::isInf(const SX& val){
  return val.isInf();
}

bool casadi_limits<SX>::isMinusInf(const SX& val){
  return val.isMinusInf();
}

bool casadi_limits<SX>::isNaN(const SX& val){
  return val.isNan();
}

SX SX::exp() const{
  return BinarySXNode::createT<EXP>(*this);
}

SX SX::log() const{
  return BinarySXNode::createT<LOG>(*this);
}

SX SX::log10() const{
  return log()*(1/std::log(10.));
}

SX SX::sqrt() const{
  if(isOne() || isZero())
    return *this;
  else if(isSquared())
    return node->dep(0).fabs();
  else
    return BinarySXNode::createT<SQRT>(*this);
}

SX SX::sin() const{
  if(node->isZero())
    return 0;
  else
    return BinarySXNode::createT<SIN>(*this);
}

SX SX::cos() const{
  if(node->isZero())
    return 1;
  else
    return BinarySXNode::createT<COS>(*this);
}

SX SX::tan() const{
  if(node->isZero())
    return 0;
  else
    return BinarySXNode::createT<TAN>(*this);
}

SX SX::arcsin() const{
  return BinarySXNode::createT<ASIN>(*this);
}

SX SX::arccos() const{
  return BinarySXNode::createT<ACOS>(*this);
}

SX SX::arctan() const{
  return BinarySXNode::createT<ATAN>(*this);
}

SX SX::sinh() const{
  if(node->isZero())
    return 0;
  else
    return BinarySXNode::createT<SINH>(*this);
}

SX SX::cosh() const{
  if(node->isZero())
    return 1;
  else
    return BinarySXNode::createT<COSH>(*this);
}

SX SX::tanh() const{
  if(node->isZero())
    return 0;
  else
    return BinarySXNode::createT<TANH>(*this);
}

SX SX::floor() const{
  return BinarySXNode::createT<FLOOR>(*this);
}

SX SX::ceil() const{
  return BinarySXNode::createT<CEIL>(*this);
}

SX SX::erf() const{
  return BinarySXNode::createT<ERF>(*this);
}

SX SX::fabs() const{
  if(isConstant() && getValue()>=0)
    return *this;
  else if(isOp(FABS))
    return *this;
  else if(isSquared())
    return *this;
  else
    return BinarySXNode::createT<FABS>(*this);
}

SX::operator Matrix<SX>() const{
  return Matrix<SX>(1,1,*this);
}

SX SX::fmin(const SX &b) const{
  return BinarySXNode::createT<FMIN>(*this,b);
}

SX SX::fmax(const SX &b) const{
  return BinarySXNode::createT<FMAX>(*this,b);
}

SX SX::printme(const SX &b) const{
  return BinarySXNode::createT<OP_PRINTME>(*this,b);
}

SX SX::__pow__(const SX& n) const{
  if(n->isConstant()) {
    if (n->isInteger()){
      int nn = n->getIntValue();
      if(nn == 0)
        return 1;
      else if(nn>100 || nn<-100) // maximum depth
        return BinarySXNode::createT<CONSTPOW>(*this,nn);
      else if(nn<0) // negative power
        return 1/pow(*this,-nn);
      else if(nn%2 == 1) // odd power
        return *this*pow(*this,nn-1);
      else{ // even power
        SX rt = pow(*this,nn/2);
        return rt*rt;
      }
    } else if(n->getValue()==0.5){
      return sqrt();
    } else {
      return BinarySXNode::createT<CONSTPOW>(*this,n);
    }
  } else {
    return BinarySXNode::createT<POW>(*this,n);
  }
}

SX SX::__constpow__(const SX& n) const{
  return BinarySXNode::createT<CONSTPOW>(*this,n);
}

SX SX::constpow(const SX& n) const{
  return BinarySXNode::createT<CONSTPOW>(*this,n);
}

int SX::getTemp() const{
  return (*this)->temp;
}
    
void SX::setTemp(int t){
  (*this)->temp = t;
}

long SX::max_num_calls_in_print_ = 10000;

void SX::setMaxNumCallsInPrint(long num){
  max_num_calls_in_print_ = num;
}

long SX::getMaxNumCallsInPrint(){
  return max_num_calls_in_print_;
}

} // namespace CasADi

using namespace CasADi;
namespace std{

SX numeric_limits<SX>::infinity() throw(){
  return CasADi::casadi_limits<SX>::inf;
}

SX numeric_limits<SX>::quiet_NaN() throw(){
  return CasADi::casadi_limits<SX>::nan;
}

SX numeric_limits<SX>::min() throw(){
  return SX(numeric_limits<double>::min());
}

SX numeric_limits<SX>::max() throw(){
  return SX(numeric_limits<double>::max());
}

SX numeric_limits<SX>::epsilon() throw(){
  return SX(numeric_limits<double>::epsilon());
}

SX numeric_limits<SX>::round_error() throw(){
  return SX(numeric_limits<double>::round_error());
}


} // namespace std

