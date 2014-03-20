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

#ifndef SYMBOLIC_OCP_HPP
#define SYMBOLIC_OCP_HPP

#include "variable.hpp"

namespace CasADi{
  
  // Forward declarations
  class XMLNode;
      
  /** \brief A flat OCP representation coupled to an XML file

      <H3>Variables:  </H3>
      \verbatim
      x:      differential states
      z:      algebraic states
      p :     independent parameters
      t :     time
      u :     control signals
      q :     quadrature states
      y :     dependent variables
      \endverbatim 

      <H3>Equations:  </H3>
      \verbatim
      explicit or implicit ODE: \dot{x} = ode(t,x,z,u,p_free,pi,pd)
      or                           0 = ode(t,x,z,\dot{x},u,p_free,pi,pd)
      algebraic equations:            0 = alg(t,x,z,u,p_free,pi,pd)
      quadratures:              \dot{q} = quad(t,x,z,u,p_free,pi,pd)
      dependent equations:            y = dep(t,x,z,u,p_free,pi,pd)
      initial equations:              0 = initial(t,x,z,u,p_free,pi,pd)
      \endverbatim 

      <H3>Objective function terms:  </H3>
      \verbatim
      Mayer terms:          \sum{mterm_k}
      Lagrange terms:       \sum{\integral{mterm}}
      \endverbatim

      Note that when parsed, all dynamic equations end up in the implicit category "dae". 
      At a later state, the DAE can be reformulated, for example in semi-explicit form, 
      possibly in addition to a set of quadrature states.
 
      The functions for reformulation is are provided as member functions to this class or as independent
      functions located in the header file "ocp_tools.hpp".

      <H3>Usage skeleton:</H3>
  
      1. Call default constructor 
      > SymbolicOCP ocp;
  
      2. Parse an FMI conformant XML file <BR>
      > ocp.parseFMI(xml_file_name)
  
      3. Modify/add variables, equations, optimization <BR>
      > ...
  
      When the optimal control problem is in a suitable form, it is possible to either generate functions
      for numeric/symbolic evaluation or exporting the OCP formulation into a new FMI conformant XML file.
      The latter functionality is not yet available.

      \date 2012
      \author Joel Andersson
  */
  class SymbolicOCP : public PrintableObject{
  public:

    /// Default constructor
    SymbolicOCP();
    
    /** @name Variables categories
     *  Public data members
     */
    //@{
    /** \brief Time */
    SX t;
    
    /** \brief Fully implicit states (includes differential states and algebraic variables) */
    SX s;

    /** \brief Differential states */
    SX x;
    
    /** \brief Algebraic variables */
    SX z;
    
    /** \brief Quadrature states (length == quad().size()) */
    SX q;

    /** \brief Independent constants */
    SX ci;

    /** \brief Dependent constants */
    SX cd;

    /** \brief Independent parameters 
        An independent parameter is a parameter whose value is determined by an expression that contains only literals: "parameter Real p1=2" or "parameter Boolean b(start=true)". In the latter case, the value of the parameter becomes true, and the Modelica compiler will generate a warning since there is no binding expression for the parameter. An independent parameter is fixed after the DAE has been initialized. */
    SX pi;

    /** \brief Dependent parameters 
        A dependent parameter is a parameter whose value is determined by an expression which contains references to other parameters: "parameter Real p2=2*p1". A dependent parameter is fixed after the DAE has been initialized. */
    SX pd;

    /** \brief Free parameters 
        A free parameter (which is Optimica specific without correspondance in Modelica) is a parameter that the optimization algorithm can change in order to minimize the cost function: "parameter Real x(free=true)". Note that these parameters in contrast to dependent/independent parameters may change after the DAE has been initialized. A free parameter should not have any binding expression since it would then no longer be free. The compiler will transform non-free parameters to free parameters if they depend on a free parameters. The "free" attribute thus propagage through the parameter binding equations. */
    SX pf;
    
    /** \brief Dependent variables (length == dep().size()) */
    SX y;
    
    /** \brief Control signals */
    SX u;

    //@}
    
    /** @name Equations
     *  Get all equations of a particular type 
     */
    //@{
      
    /// Fully implific DAE
    SX dae;

    /// Explicit ODE
    SX ode;
    
    /// Algebraic constraints
    SX alg;
    
    /// Quadrature equations
    SX quad;
    
    /// Dependent equations
    SX dep;
    
    /// Initial equations (remove?)
    SX initial;
    //@}
    
    /** @name Time points
     */
    //@{

    /// Interval start time
    double t0;
    
    /// Interval final time
    double tf;
    
    /// Interval start time is free
    bool t0_free;
    
    /// Interval final time is free
    bool tf_free;
    
    /// Interval start time initial guess
    double t0_guess;
    
    /// Interval final time initial guess
    double tf_guess;
    
    /// Time points
    std::vector<double> tp;
    
    //@}

    /** @name Objective function terms
     *  Terms in the objective function.
     */
    //@{
      
    /// Mayer terms in the objective (point terms)
    SX mterm;
    
    /// Lagrange terms in the objective (integral terms)
    SX lterm;
    //@}

    /** @name Path constraints of the optimal control problem
     */
    //@{

    /// Path constraint functions
    SX path;
    
    /// Path constraint functions bounds
    DMatrix path_min, path_max;
    //@}

    /** @name Point constraints of the optimal control problem
     */
    //@{

    /// Point constraint functions
    SX point;
    
    /// Path constraint functions bounds
    DMatrix point_min, point_max;
    //@}

    /// Parse from XML to C++ format
    void parseFMI(const std::string& filename);

    /// Add a variable
    void addVariable(const std::string& name, const Variable& var);
    
    //@{
    /// Access a variable by name
    Variable& variable(const std::string& name);
    const Variable& variable(const std::string& name) const;
    //@}
    
    /** @name Manipulation
     *  Reformulate the dynamic optimization problem.
     */
    //@{
    /// Eliminate interdependencies in the dependent equations
    void eliminateInterdependencies();
    
    /// Eliminate dependent equations, by default sparing the dependent variables with upper or lower bounds
    void eliminateDependent(bool eliminate_dependents_with_bounds=true);

    /// Eliminate Lagrange terms from the objective function and make them quadrature states
    void eliminateLagrangeTerms();
    
    /// Eliminate quadrature states and turn them into ODE states
    void eliminateQuadratureStates();
    
    /// Identify the algebraic variables and separate them from the states
    void identifyALG();

    /// Sort the DAE and implicly defined states
    void sortDAE();

    /// Sort the algebraic equations and algebraic states
    void sortALG();

    /// Sort the dependent parameters
    void sortDependentParameters();
    
    /// Transform the implicit ODE to an explicit ODE
    void makeExplicit();
    
    /// Eliminate algebraic states, transforming them into outputs
    void eliminateAlgebraic();
    
    /// Substitute the dependents from a set of expressions
    std::vector<SX> substituteDependents(const std::vector<SX>& x) const;
    
    /// Generate a MUSCOD-II compatible DAT file
    void generateMuscodDatFile(const std::string& filename, const Dictionary& mc2_ops=Dictionary()) const;
    
    //@}

    /// Scale the variables
    void scaleVariables();
    
    /// Scale the implicit equations
    void scaleEquations();

    /// Find an expression by name
    SX operator()(const std::string& name) const;

    /// Find an derivative expression by name
    SX der(const std::string& name) const;

    /// Find an derivative expression by non-differentiated expression
    SX der(const SX& var) const;

    /// Get the nominal value for a component
    double nominal(const std::string& name) const;
    
    /// Get the nominal values given a vector of symbolic variables
    std::vector<double> nominal(const SX& var) const;

    /// Set the nominal value for a component
    void setNominal(const std::string& name, double val);

    /// Get the lower bound for a component
    double min(const std::string& name, bool nominal=false) const;

    /// Get the lower bound given a vector of symbolic variables
    std::vector<double> min(const SX& var, bool nominal=false) const;

    /// Set the upper bound for a component
    void setMin(const std::string& name, double val);

    /// Get the upper bound for a component
    double max(const std::string& name, bool nominal=false) const;

    /// Get the upper bound given a vector of symbolic variables
    std::vector<double> max(const SX& var, bool nominal=false) const;

    /// Set the upper bound for a component
    void setMax(const std::string& name, double val);

    /// Get the value at time 0 for a component
    double start(const std::string& name, bool nominal=false) const;

    /// Get the value at time 0 given a vector of symbolic variables
    std::vector<double> start(const SX& var, bool nominal=false) const;

    /// Set the value at time 0 for a component
    void setStart(const std::string& name, double val);

    /// Set the value at time 0 for a component
    void setStart(const SX& var, const std::vector<double>& val);

    /// Get the initial guess for a component
    double initialGuess(const std::string& name, bool nominal=false) const;

    /// Get the initial guess given a vector of symbolic variables
    std::vector<double> initialGuess(const SX& var, bool nominal=false) const;

    /// Set the initial guess for a component
    void setInitialGuess(const std::string& name, double val);

    /// Get the derivative at time 0 for a component
    double derivativeStart(const std::string& name, bool nominal=false) const;

    /// Get the derivative at time 0 given a vector of symbolic variables
    std::vector<double> derivativeStart(const SX& var, bool nominal=false) const;

    /// Set the derivative at time 0 for a component
    void setDerivativeStart(const std::string& name, double val);

    /// Get the unit for a component
    std::string unit(const std::string& name) const;

    /// Get the unit given a vector of symbolic variables (all units must be identical)
    std::string unit(const SX& var) const;

    /// Set the unit for a component
    void setUnit(const std::string& name, const std::string& val);

    /// Timed variable (never allocate)
    SX atTime(const std::string& name, double t, bool allocate=false) const;

    /// Timed variable (allocate if necessary)
    SX atTime(const std::string& name, double t, bool allocate=false);

#ifndef SWIG
    ///  Print representation
    virtual void repr(std::ostream &stream=std::cout) const;

    /// Print description 
    virtual void print(std::ostream &stream=std::cout) const;

    // Internal methods
  protected:

    /// Get the qualified name
    static std::string qualifiedName(const XMLNode& nn);
    
    /// Find of variable by name
    std::map<std::string,Variable> varmap_;

    /// Read an equation
    SX readExpr(const XMLNode& odenode);

    /// Read a variable
    Variable& readVariable(const XMLNode& node);

#endif // SWIG

  };

} // namespace CasADi

#endif //SYMBOLIC_OCP_HPP
