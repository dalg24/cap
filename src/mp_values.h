#ifndef CAP_MP_VALUES_H
#define CAP_MP_VALUES_H

//#include <deal.II/base/types.h>
//#include <deal.II/base/tensor.h>
//#include <deal.II/dofs/dof_handler.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/dofs/dof_accessor.h>
#include <boost/property_tree/ptree.hpp>

namespace cap {

//////////////////////// MP VALUES PARAMETERS ////////////////////////////
template <int dim, int spacedim=dim>
class MPValuesParameters {
public:
    MPValuesParameters(std::shared_ptr<boost::property_tree::ptree> d)
        : database(d)
    { }
    virtual ~MPValuesParameters() 
    { }
// keep public for now
    std::shared_ptr<boost::property_tree::ptree> database;

};

//////////////////////// MP VALUES ////////////////////////////
template <int dim, int spacedim=dim>
class MPValues {
public:
    typedef typename dealii::DoFHandler<dim, spacedim>::active_cell_iterator active_cell_iterator;
    MPValues(MPValuesParameters<dim, spacedim> const & ) { }
    virtual ~MPValues() { }
    virtual void get_values(std::string const &          key,
                            active_cell_iterator const & cell,
                            std::vector<double> &        values) const;
    virtual void get_values(std::string const &                         key,
                            active_cell_iterator const &                cell,
                            std::vector<dealii::Tensor<1, spacedim> > & values) const;
};


//////////////////////// SUPERCAPACITOR MP VALUES ////////////////////////////
template <int dim, int spacedim=dim>
class SuperCapacitorMPValuesParameters : public MPValuesParameters<dim, spacedim> {
public:
    SuperCapacitorMPValuesParameters(std::shared_ptr<boost::property_tree::ptree> d)
        : MPValuesParameters<dim, spacedim>(d)
    { }
};

template <int dim, int spacedim=dim>
class SuperCapacitorMPValues : public MPValues<dim, spacedim> { 
public:
    typedef typename dealii::DoFHandler<dim, spacedim>::active_cell_iterator active_cell_iterator;
    SuperCapacitorMPValues(MPValuesParameters<dim, spacedim> const & parameters);
    void get_values(std::string const &          key,
                    active_cell_iterator const & cell,
                    std::vector<double> &        values) const override;
protected:
    dealii::types::material_id separator_material_id;
    dealii::types::material_id anode_electrode_material_id;
    dealii::types::material_id anode_collector_material_id;
    dealii::types::material_id cathode_electrode_material_id;
    dealii::types::material_id cathode_collector_material_id;

    double separator_thermal_conductivity;
    double electrode_thermal_conductivity;
    double collector_thermal_conductivity;
    double separator_density;
    double electrode_density;
    double collector_density;
    double electrolyte_mass_density;
    double separator_heat_capacity;
    double electrode_heat_capacity;
    double collector_heat_capacity;

    double differential_capacitance;
    double electrode_void_volume_fraction;
    double separator_void_volume_fraction;
    double electrolyte_conductivity;
    double solid_phase_conductivity;
    double collector_electrical_resistivity;
    double separator_tortuosity_factor;
    double electrode_tortuosity_factor;
    double bruggemans_coefficient;
    double pores_characteristic_dimension;
    double pores_geometry_factor;

    double anodic_charge_transfer_coefficient;
    double cathodic_charge_transfer_coefficient;
    double faraday_constant;
    double gas_constant;
    double exchange_current_density;
    double temperature;
};

} // end namespace cap

#endif // CAP_MP_VALUES_H
