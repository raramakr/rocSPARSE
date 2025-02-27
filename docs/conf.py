# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

from rocm_docs import ROCmDocs


external_toc_path = "./sphinx/_toc.yml"

external_projects_current_project = "rocsparse"

docs_core = ROCmDocs("rocSPARSE Documentation")
docs_core.run_doxygen(doxygen_root="doxygen", doxygen_path="doxygen/docBin/xml")
docs_core.setup()

for sphinx_var in ROCmDocs.SPHINX_VARS:
    globals()[sphinx_var] = getattr(docs_core, sphinx_var)
