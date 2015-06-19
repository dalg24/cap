#define BOOST_TEST_MODULE TestRagoneChart
#define BOOST_TEST_MAIN
#include <cap/resistor_capacitor.h>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <cmath>



namespace cap {

std::function<void(double const, double const, std::shared_ptr<cap::EnergyStorageDevice>)>
get_discharge_evolve_one_time_step(std::string const & discharge_mode, std::shared_ptr<boost::property_tree::ptree const> database)
{
    if (discharge_mode.compare("constant_current") == 0) {
        double const discharge_current = database->get<double>("discharge_current");
        return [discharge_current](double const, double const time_step, std::shared_ptr<cap::EnergyStorageDevice> dev)
            { dev->evolve_one_time_step_constant_current(time_step, -discharge_current); };
    } else if (discharge_mode.compare("constant_power") == 0) {
        double const discharge_power = database->get<double>("discharge_power");
        return [discharge_power](double const, double const time_step, std::shared_ptr<cap::EnergyStorageDevice> dev)
            { dev->evolve_one_time_step_constant_power(time_step, -discharge_power); };
    } else if (discharge_mode.compare("constant_load") == 0) {
        double const discharge_load = database->get<double>("discharge_load");
        return [discharge_load](double const, double const time_step, std::shared_ptr<cap::EnergyStorageDevice> dev)
            { dev->evolve_one_time_step_constant_load(time_step, discharge_load); };
    } else {
        throw std::runtime_error("invalid discharge mode "+discharge_mode);
    }
}



std::tuple<double, double>
find_power_energy(std::shared_ptr<cap::EnergyStorageDevice> dev, std::shared_ptr<boost::property_tree::ptree> database)
{
    std::string const discharge_mode = database->get<std::string>("discharge_mode");
    auto evolve_one_time_step = get_discharge_evolve_one_time_step(discharge_mode, database);

    double const initial_voltage    = database->get<double>("initial_voltage"   );
    double       time_step          = database->get<double>("time_step"         );
    double const final_voltage      = database->get<double>("final_voltage"     ); // end criterion

    int const min_steps = database->get<int>("min_steps_per_discharge");
    int const max_steps = database->get<int>("max_steps_per_discharge");
    double       voltage;
    double       current;
    double       energy;
    double       time;
    int          step;
    for (int i = 0; i < 2; ++i)
    {
        step   = 0;
        time   = 0.0;
        energy = 0.0;
        dev->reset_voltage(initial_voltage);
        for (voltage = initial_voltage ; voltage >= final_voltage; )
        {
            ++step;
            evolve_one_time_step(time, time_step, dev);
            time += time_step;
            dev->get_voltage(voltage);
            dev->get_current(current);
            energy -= voltage * current * time_step;
        }
        if (step >= min_steps)
            break;
        else
            time_step = time / max_steps;
    }
    database->put("discharge_time", time     );
    database->put("steps"         , step     );
    database->put("time_step"     , time_step);
    return std::make_tuple(energy / time, energy);
}



std::function<void(std::shared_ptr<boost::property_tree::ptree>)>
get_initialize(std::shared_ptr<boost::property_tree::ptree const> database)
{
    std::string const discharge_mode = database->get<std::string>("discharge_mode");
    if (discharge_mode.compare("constant_current") == 0) {
        double const discharge_current_lower_limit = database->get<double>("discharge_current_lower_limit");
        return
            [discharge_current_lower_limit](std::shared_ptr<boost::property_tree::ptree> d)
            {
                d->put("discharge_current", discharge_current_lower_limit);
            };
    } else if (discharge_mode.compare("constant_power") == 0) {
        double const discharge_power_lower_limit = database->get<double>("discharge_power_lower_limit");
        return
            [discharge_power_lower_limit](std::shared_ptr<boost::property_tree::ptree> d)
            {
                d->put("discharge_power", discharge_power_lower_limit);
            };
    } else {
        throw std::runtime_error("invalid discharge mode "+discharge_mode);
    }
}



std::function<bool(std::shared_ptr<boost::property_tree::ptree const>)>
get_condition(std::shared_ptr<boost::property_tree::ptree const> database)
{
    std::string const discharge_mode = database->get<std::string>("discharge_mode");
    if (discharge_mode.compare("constant_current") == 0) {
        double const discharge_current_upper_limit = database->get<double>("discharge_current_upper_limit");
        return
            [discharge_current_upper_limit](std::shared_ptr<boost::property_tree::ptree const> d)
            {
                double const discharge_current = d->get<double>("discharge_current");
                return discharge_current <= discharge_current_upper_limit;
            };
    } else if (discharge_mode.compare("constant_power") == 0) {
        double const discharge_power_upper_limit = database->get<double>("discharge_power_upper_limit");
        return
            [discharge_power_upper_limit](std::shared_ptr<boost::property_tree::ptree const> d)
            {
                double const discharge_power = d->get<double>("discharge_power");
                return discharge_power <= discharge_power_upper_limit;
            };
    } else {
        throw std::runtime_error("invalid discharge mode "+discharge_mode);
    }
}



std::function<void(std::shared_ptr<boost::property_tree::ptree>)>
get_increase(std::shared_ptr<boost::property_tree::ptree const> database)
{
    std::string const discharge_mode = database->get<std::string>("discharge_mode");
    if (discharge_mode.compare("constant_current") == 0) {
        int const steps_per_decade = database->get<int>("steps_per_decade");
        return
            [steps_per_decade](std::shared_ptr<boost::property_tree::ptree> d)
            {
                double discharge_current = d->get<double>("discharge_current");
                discharge_current *= std::pow(10.0, 1.0/steps_per_decade);
                d->put("discharge_current", discharge_current);
            };
    } else if (discharge_mode.compare("constant_power") == 0) {
        int const steps_per_decade = database->get<int>("steps_per_decade");
        return
            [steps_per_decade](std::shared_ptr<boost::property_tree::ptree> d)
            {
                double discharge_power = d->get<double>("discharge_power");
                discharge_power *= std::pow(10.0, 1.0/steps_per_decade);
                d->put("discharge_power", discharge_power);
            };
    } else {
        throw std::runtime_error("invalid discharge mode "+discharge_mode);
    }

}



void scan(std::shared_ptr<cap::EnergyStorageDevice> dev, std::shared_ptr<boost::property_tree::ptree const> database, std::ostream & os = std::cout)
{
    auto initialize = get_initialize(database);
    auto condition  = get_condition (database);
    auto increase   = get_increase  (database);
    double time;
    double power;
    double energy;
    int    steps;
    std::shared_ptr<boost::property_tree::ptree> dummy_database =
        std::make_shared<boost::property_tree::ptree>(*database);
    for (initialize(dummy_database); condition(dummy_database); increase(dummy_database))
    {
         try
        {
            std::tie(power, energy) =
                cap::find_power_energy(dev, dummy_database);
        }
        catch(std::exception & e)
        {
            std::cerr<<power<<"  "<<e.what()<<"\n";
            break;
        }
        time  = dummy_database->get<double>("discharge_time");
        steps = dummy_database->get<int   >("steps"         );
        if (steps <= 1)
            break;

        os<<boost::format("  %10.7e  %10.7e  %10.7e  %10d \n")
            % power
            % energy
            % time
            % steps
            ;
    }
}

} // end namespace cap



BOOST_AUTO_TEST_CASE( constant_power_vs_constant_current )
{
    // parse input file
    std::shared_ptr<boost::property_tree::ptree> input_database =
        std::make_shared<boost::property_tree::ptree>();
    read_xml("input_ragone_chart", *input_database);

    // build an energy storage system
    std::shared_ptr<boost::property_tree::ptree> device_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("device"));
    std::shared_ptr<cap::EnergyStorageDevice> device =
        cap::buildEnergyStorageDevice(std::make_shared<cap::Parameters>(device_database));

    // scan the system with constant current discharge
    std::fstream fout;
    fout.open("ragone_chart_constant_current_data", std::fstream::out);

    std::shared_ptr<boost::property_tree::ptree> ragone_chart_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("ragone_chart_constant_current"));
    cap::scan(device, ragone_chart_database, fout);

    // scan the system with constant power discharge
    fout.close();
    fout.open("ragone_chart_constant_power_data", std::fstream::out);

    ragone_chart_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("ragone_chart_constant_power"));
    cap::scan(device, ragone_chart_database, fout);
}    



BOOST_AUTO_TEST_CASE( test_ragone_chart_constant_power )
{
    // parse input file
    std::shared_ptr<boost::property_tree::ptree> input_database =
        std::make_shared<boost::property_tree::ptree>();
    read_xml("input_ragone_chart", *input_database);

    std::string const type = input_database->get<std::string>("device.type");
    // current test will only work for series or parallel rc circuit
    if ((type.compare("SeriesRC") != 0) && (type.compare("ParallelRC") != 0))
        throw std::runtime_error("test measure impedance check not implemented for "+type);

    // build an energy storage system
    std::shared_ptr<boost::property_tree::ptree> device_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("device"));
    std::shared_ptr<cap::EnergyStorageDevice> device =
        cap::buildEnergyStorageDevice(std::make_shared<cap::Parameters>(device_database));

    double const power_lower_limit   = input_database->get<double>("ragone_chart_constant_power.discharge_power_lower_limit");
    double const power_upper_limit   = input_database->get<double>("ragone_chart_constant_power.discharge_power_upper_limit");
    int    const steps_per_decade    = input_database->get<int   >("ragone_chart_constant_power.steps_per_decade" );
    double const initial_voltage     = input_database->get<double>("ragone_chart_constant_power.initial_voltage"  );
    double const final_voltage       = input_database->get<double>("ragone_chart_constant_power.final_voltage"    );
    double const series_resistance   = input_database->get<double>("device.series_resistance"                     );
    double const parallel_resistance = input_database->get<double>("device.parallel_resistance"                   );
    double const capacitance         = input_database->get<double>("device.capacitance"                           );

    std::shared_ptr<boost::property_tree::ptree> ragone_chart_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("ragone_chart_constant_power"));
    std::fstream fout;
    fout.open("ragone_chart_data3", std::fstream::out);
    for (double power = power_lower_limit; power <= power_upper_limit; power *= std::pow(10.0, 1.0/steps_per_decade))
    {
        ragone_chart_database->put("discharge_power", power);
        double const tmp = 0.5 * initial_voltage + std::sqrt(initial_voltage*initial_voltage / 4.0 + series_resistance * (-1.0 * power));
        double const tmp2 = tmp * tmp;
        double const tmp3 =
            (final_voltage*final_voltage / parallel_resistance - (-1.0 * power) * (1.0 + series_resistance / parallel_resistance))
            /
            (tmp2 / parallel_resistance - (-1.0 * power) * (1.0 + series_resistance / parallel_resistance))
            ;
        double const tmp4 = tmp3 * tmp2 / (final_voltage*final_voltage);
        double const exact_energy =
            (
                (type.compare("SeriesRC") == 0)
                ?
                -0.5 * capacitance * (series_resistance * (-1.0 * power) * std::log(final_voltage*final_voltage / tmp2) + tmp2 - final_voltage*final_voltage)
                :
                -0.5 * capacitance * (-1.0 * power) * (
                    parallel_resistance * std::log(tmp3)
                    +
                    parallel_resistance * series_resistance / (parallel_resistance + series_resistance) * std::log(tmp4)
                    )
            );
        double const exact_power = - power;
        double const exact_time  = exact_energy / exact_power;
        double       computed_energy;
        double       computed_power;
        ragone_chart_database->put("discharge_power", power);
try
{
        std::tie(computed_power, computed_energy) =
            cap::find_power_energy(device, ragone_chart_database);
}
catch(std::exception & e)
{
    std::cerr<<power<<"  "<<e.what()<<"\n";
    break;
}
        double const computed_time = computed_energy / computed_power;
        double const time_step     = ragone_chart_database->get<double>("time_step");
        int    const steps         = ragone_chart_database->get<double>("steps"    );
        int    const min_steps     = ragone_chart_database->get<double>("min_steps_per_discharge");
        BOOST_CHECK_GE(steps, min_steps);
        BOOST_CHECK_SMALL(computed_time  - exact_time  , time_step);
        BOOST_CHECK_CLOSE(computed_power , -exact_power , 1.0e-6);
        BOOST_CHECK_CLOSE(computed_power , power        , 1.0e-6);
        BOOST_CHECK_CLOSE(computed_energy, -exact_energy, 100.0 * time_step / exact_time);
        fout<<boost::format("  %10.7e  %10.7e  %10.7e  %10d \n")
            % -exact_power
            % -exact_energy
            % exact_time
            % 0
            ;
    }

}    



BOOST_AUTO_TEST_CASE( test_ragone_chart_constant_current )
{
    // parse input file
    std::shared_ptr<boost::property_tree::ptree> input_database =
        std::make_shared<boost::property_tree::ptree>();
    read_xml("input_ragone_chart", *input_database);

    std::string const type = input_database->get<std::string>("device.type");
    // current test will only work for series or parallel rc circuit
    if ((type.compare("SeriesRC") != 0) && (type.compare("ParallelRC") != 0))
        throw std::runtime_error("test measure impedance check not implemented for "+type);

    // build an energy storage system
    std::shared_ptr<boost::property_tree::ptree> device_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("device"));
    std::shared_ptr<cap::EnergyStorageDevice> device =
        cap::buildEnergyStorageDevice(std::make_shared<cap::Parameters>(device_database));

    double const current_lower_limit   = input_database->get<double>("ragone_chart_constant_current.discharge_current_lower_limit");
    double const current_upper_limit   = input_database->get<double>("ragone_chart_constant_current.discharge_current_upper_limit");
    int    const steps_per_decade      = input_database->get<int   >("ragone_chart_constant_current.steps_per_decade"   );
    double const initial_voltage       = input_database->get<double>("ragone_chart_constant_current.initial_voltage"    );
    double const final_voltage         = input_database->get<double>("ragone_chart_constant_current.final_voltage"      );
    double const series_resistance     = input_database->get<double>("device.series_resistance"                         );
    double const parallel_resistance   = input_database->get<double>("device.parallel_resistance"                       );
    double const capacitance           = input_database->get<double>("device.capacitance"                               );

    std::shared_ptr<boost::property_tree::ptree> ragone_chart_database =
        std::make_shared<boost::property_tree::ptree>(input_database->get_child("ragone_chart_constant_current"));
    std::fstream fout;
    fout.open("ragone_chart_data4", std::fstream::out);
    for (double current = current_lower_limit; current <= current_upper_limit; current *= std::pow(10.0, 1.0/steps_per_decade))
    {
        ragone_chart_database->put("discharge_current", current);
        double const time =
            (
                (type.compare("SeriesRC") == 0)
                ?
                (final_voltage - initial_voltage - series_resistance * (-1.0 * current)) * capacitance / (-1.0 * current)
                :
                - parallel_resistance * capacitance * std::log(
                    (final_voltage - (series_resistance + parallel_resistance) * (-1.0 * current))
                    /
                    (initial_voltage - parallel_resistance * (-1.0 * current))
                )
            );
        double const exact_energy = 
            (
                (type.compare("SeriesRC") == 0)
                ?
                (series_resistance * current * current + initial_voltage * (-1.0 * current)) * time
                    + current * current * 0.5 / capacitance * time * time
                :
                (series_resistance + parallel_resistance) * current * current * time 
                    + (initial_voltage * (-1.0 * current) - parallel_resistance * current * current)
                        * (-1.0 * parallel_resistance * capacitance)
                        * std::expm1(
                            - time / (parallel_resistance * capacitance)
                        )
            );
        double const exact_time  = time;
        double const exact_power = exact_energy / exact_time;
        double       computed_energy;
        double       computed_power;
        ragone_chart_database->put("discharge_current", current);
        std::tie(computed_power, computed_energy) =
            cap::find_power_energy(device, ragone_chart_database);
        double const computed_time = computed_energy / computed_power;
        double const time_step     = ragone_chart_database->get<double>("time_step");
        int    const steps         = ragone_chart_database->get<double>("steps"    );
        int    const min_steps     = ragone_chart_database->get<double>("min_steps_per_discharge");
        if (steps == 1)
            break;
        BOOST_CHECK_GE(steps, min_steps);
        BOOST_CHECK_SMALL(computed_time  - exact_time  , time_step);
        BOOST_CHECK_CLOSE(computed_power , -exact_power , 100.0 * std::sqrt(2.0) * time_step / exact_time);
        BOOST_CHECK_CLOSE(computed_energy, -exact_energy, 100.0 * time_step / exact_time);
        fout<<boost::format("  %10.7e  %10.7e  %10.7e  %10d \n")
            % -exact_power
            % -exact_energy
            % exact_time
            % 0
            ;
    }

}    
