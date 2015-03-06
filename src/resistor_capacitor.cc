#include <cap/resistor_capacitor.h>
#include <boost/format.hpp>
#include <cmath>
#include <stdexcept>

namespace cap {



SeriesRC::
SeriesRC(std::shared_ptr<Parameters const> params)
{
    std::shared_ptr<boost::property_tree::ptree const> database = params->database;
    R   = database->get<double>("series_resistance"             );
    C   = database->get<double>("capacitance"                   );
    U_C = database->get<double>("initial_capacitor_voltage", 0.0);
    I   = database->get<double>("initial_current",           0.0);
    U   = U_C + R * I;
}



SeriesRC::
SeriesRC(double const resistance, double const capacitance, double const initial_capacitor_voltage)
    : R(resistance)
    , C(capacitance)
    , U_C(initial_capacitor_voltage) 
    , U(U_C)
    , I(0.0) 
{ }



void
SeriesRC::
print_data(std::ostream & os) const
{
    os<<boost::format("  %10.7f  %10.7f  %10.7f  \n")
        % I
        % U
        % U_C
    ;
}



void
SeriesRC::
reset_voltage(double const voltage)
{
    U_C = voltage;
    U   = U_C;
    I   = 0.0;
}



void
SeriesRC::
reset_current(double const current)
{
    I = current;
    U = U_C + R * I;
}



void
SeriesRC::
reset(double const capacitor_voltage)
{
    U_C = capacitor_voltage;
    U = U_C + R * I;
}



void
SeriesRC::
evolve_one_time_step_constant_power(double const delta_t, double const constant_power)
{
    evolve_one_time_step_constant_power(delta_t, constant_power, "toto");
}



void
SeriesRC::
evolve_one_time_step_constant_current(double const delta_t, double const constant_current)
{
    I = constant_current;
    U_C += I * delta_t / C;
    U = R * I + U_C;
}



void
SeriesRC::
evolve_one_time_step_constant_voltage(double const delta_t, double const constant_voltage)
{
    U = constant_voltage;
    U_C = U - (U - U_C) * std::exp(-delta_t/(R*C));
    I = U - U_C / R;
}



std::size_t
SeriesRC::
evolve_one_time_step_constant_power(double const delta_t, double const constant_power, std::string const & method)
{
    // TODO: if P is zero do constant current 0
    double const P = constant_power;
    double const      ATOL  = 1.0e-14;
    double const      RTOL  = 1.0e-14;
    std::size_t const MAXIT = 30;
    double const TOL = std::abs(P) * RTOL + ATOL;
    size_t k = 0;
    while (true)
    {
        ++k;
        I = P / U;
        if (method.compare("FIXED_POINT") == 0)
            U = (R + delta_t / C) * I + U_C;
        else if (method.compare("NEWTON") == 0)
            U += ((R + delta_t / C) * P / U - U + U_C) / ((R + delta_t / C) * P / (U * U) + 1.0);
        else
            throw std::runtime_error("invalid method "+method);
        if (std::abs(P - U * I) < TOL)
            break;
        if (k >= MAXIT)
            throw std::runtime_error(method+" fail to converge within "+std::to_string(MAXIT)+" iterations");
    }
    U_C += I * delta_t / C;
    return k;
}



ParallelRC::
ParallelRC(std::shared_ptr<Parameters const> params)
{
    std::shared_ptr<boost::property_tree::ptree const> database = params->database;
    R_series   = database->get<double>("series_resistance"             );
    R_parallel = database->get<double>("parallel_resistance"           );
    C          = database->get<double>("capacitance"                   );
    U_C        = database->get<double>("initial_capacitor_voltage", 0.0);
    I          = database->get<double>("initial_current",           0.0);
    U          = U_C + R_series * I;
}



ParallelRC::
ParallelRC(double const parallel_resistance, double const capacitance, double const initial_capacitor_voltage)
    : R_parallel(parallel_resistance)
    , C(capacitance)
    , U_C(initial_capacitor_voltage)
    , U(U_C)
    , I(0.0)
    , R_series(0.0)
{ }



void
ParallelRC::
print_data(std::ostream & os) const
{
    os<<boost::format("  %10.7f  %10.7f  %10.7f  \n")
        % I
        % U
        % U_C
    ;
}



void
ParallelRC::
reset_voltage(double const voltage)
{
    U_C = voltage;
    U = R_series * I + U_C;
}



void
ParallelRC::
reset_current(double const current)
{
    I   = current;
    U = R_series * I + U_C;
}



void
ParallelRC::
reset(double const capacitor_voltage)
{
    U_C = capacitor_voltage;
    U = R_series * I + U_C;
}



void
ParallelRC::
evolve_one_time_step_constant_current(double const delta_t, double const constant_current)
{
    I   = constant_current;
    U_C = R_parallel * I + (U_C - R_parallel * I) * std::exp(- delta_t / (R_parallel * C));
    U   = R_series * I + U_C;
}



void
ParallelRC::
evolve_one_time_step_constant_voltage(double const delta_t, double const constant_voltage)
{
    U   = constant_voltage;
    U_C =  R_parallel / (R_series + R_parallel) * U + (U_C - R_parallel / (R_series + R_parallel) * U) * std::exp(- delta_t * (R_series + R_parallel) / (R_series * R_parallel * C));
    I   = (U - U_C) / R_series;
}



void
ParallelRC::
evolve_one_time_step_constant_power(double const delta_t, double const constant_power)
{
    evolve_one_time_step_constant_power(delta_t, constant_power, "NEWTON");
}



std::size_t
ParallelRC::
evolve_one_time_step_constant_power(double const delta_t, double const constant_power, std::string const & method)
{
    // TODO: if P is zero do constant current 0
    double const P = constant_power;
    double const      ATOL  = 1.0e-14;
    double const      RTOL  = 1.0e-14;
    std::size_t const MAXIT = 30;
    double const TOL = std::abs(P) * RTOL + ATOL;
    size_t k = 0;
    while (true)
    {
        ++k;
        I = P / U;
        if (method.compare("FIXED_POINT") == 0)
            U = (R_series + R_parallel) * I + (U_C - R_parallel * I) * std::exp(- delta_t / (R_parallel * C));
        else if (method.compare("NEWTON") == 0)
            U += ((R_series + R_parallel * (1.0 - std::exp(- delta_t / (R_parallel * C)))) * P / U - U + U_C * std::exp(- delta_t / (R_parallel * C))) / ((R_series + R_parallel * (1.0 - std::exp(- delta_t / (R_parallel * C)))) * P / (U * U) + 1.0);
        else
            throw std::runtime_error("invalid method "+method);
        if (std::abs(P - U * I) < TOL)
            break;
        if (k >= MAXIT)
            throw std::runtime_error(method+" fail to converge within "+std::to_string(MAXIT)+" iterations");
    }
    U_C = U - R_series * I;
    return k;
}

} // end namespace