// @HEADER
// ****************************************************************************
//                Tempus: Copyright (2017) Sandia Corporation
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ****************************************************************************
// @HEADER

#ifndef Tempus_StepperForwardEuler_decl_hpp
#define Tempus_StepperForwardEuler_decl_hpp

#include "Tempus_config.hpp"
#include "Tempus_Stepper.hpp"

namespace Tempus {


/** \brief Forward Euler time stepper.
 *
 *  For the explicit ODE system,
 *  \f[
 *    \dot{x} = \bar{f}(x,t),
 *  \f]
 *  the Forward Euler stepper can be written as
 *  \f[
 *    x_{n} = x_{n-1} + \Delta t\, \bar{f}(x_{n-1},t_{n-1})
 *  \f]
 *  Forward Euler is an explicit time stepper (i.e., no solver used).
 *  Note that the time derivative by definition is
 *  \f[
 *    \dot{x}_{n} = \bar{f}(x_{n},t_{n}),
 *  \f]
 *
 *  <b> Algorithm </b>
 *  The single-timestep algorithm for Forward Euler is simply,
 *   - Evaluate \f$\bar{f}(x_{n-1},t_{n-1})\f$
 *   - \f$x_{n} \leftarrow x_{n-1} + \Delta t\, \bar{f}(x_{n-1},t_{n-1})\f$
 *   - \f$\dot{x}_n \leftarrow \bar{f}(x_{n},t_{n})\f$ [Optional]
 */
template<class Scalar>
class StepperForwardEuler : virtual public Tempus::Stepper<Scalar>
{
public:

  /// Constructor
  StepperForwardEuler(
    const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >& appModel,
    Teuchos::RCP<Teuchos::ParameterList> pList = Teuchos::null);

  /// \name Basic stepper methods
  //@{
    virtual void setModel(
      const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >& appModel);
    virtual void setNonConstModel(
      const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> >& appModel);
    virtual Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >
      getModel(){return appModel_;}

    virtual void setSolver(std::string solverName);
    virtual void setSolver(
      Teuchos::RCP<Teuchos::ParameterList> solverPL=Teuchos::null);
    virtual void setSolver(
        Teuchos::RCP<Thyra::NonlinearSolverBase<Scalar> > solver);

    /// Initialize during construction and after changing input parameters.
    virtual void initialize(){}

    /// Take the specified timestep, dt, and return true if successful.
    virtual void takeStep(
      const Teuchos::RCP<SolutionHistory<Scalar> >& solutionHistory);

    /// Get a default (initial) StepperState
    virtual Teuchos::RCP<Tempus::StepperState<Scalar> > getDefaultStepperState();
    virtual Scalar getOrder() const {return 1.0;}
    virtual Scalar getOrderMin() const {return 1.0;}
    virtual Scalar getOrderMax() const {return 1.0;}
  //@}

  /// \name ParameterList methods
  //@{
    void setParameterList(const Teuchos::RCP<Teuchos::ParameterList> & pl);
    Teuchos::RCP<Teuchos::ParameterList> getNonconstParameterList();
    Teuchos::RCP<Teuchos::ParameterList> unsetParameterList();
    Teuchos::RCP<const Teuchos::ParameterList> getValidParameters() const;
    Teuchos::RCP<Teuchos::ParameterList> getDefaultParameters() const;
  //@}

  /// \name Overridden from Teuchos::Describable
  //@{
    virtual std::string description() const;
    virtual void describe(Teuchos::FancyOStream        & out,
                          const Teuchos::EVerbosityLevel verbLevel) const;
  //@}

private:

  /// Default Constructor -- not allowed
  StepperForwardEuler();

protected:

  Teuchos::RCP<Teuchos::ParameterList>               stepperPL_;
  /// Explicit ODE ModelEvaluator
  Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > appModel_;

  Thyra::ModelEvaluatorBase::InArgs<Scalar>  inArgs_;
  Thyra::ModelEvaluatorBase::OutArgs<Scalar> outArgs_;
};
} // namespace Tempus

#endif // Tempus_StepperForwardEuler_decl_hpp
