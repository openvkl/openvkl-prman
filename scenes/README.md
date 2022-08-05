
You will need to place the appropriate .vdb files in this directory in order to
render the provided .rib files:

- smoke.rib: requires smoke.vdb, which can be found with the OpenVDB "smoke1"
  sample model at https://www.openvdb.org/download/

- wdas_cloud.rib: requires wdas_cloud.vdb, which can be found at
  https://media.disneyanimation.com/uploads/production/data_set_asset/1/asset/Cloud_Readme.pdf

Note that these .rib files are parameterized for the `impl_openvdb` plugin.
These can be run with the Intel® Open VKL plugin for RenderMan* either by
updating the parameterization or by using the RIF filter plugin `rif_benchmark`;
both techniques are described in the top-level `README.md`.
