/*
 * HDF5_loader_vector_dist.hpp
 *
 *  Created on: May 1, 2017
 *      Author: i-bird
 */

#ifndef OPENFPM_IO_SRC_HDF5_WR_HDF5_WRITER_VD_HPP_
#define OPENFPM_IO_SRC_HDF5_WR_HDF5_WRITER_VD_HPP_

#include "Packer_Unpacker/Pack_selector.hpp"
#include "Packer_Unpacker/Packer.hpp"
#include "Packer_Unpacker/Unpacker.hpp"

template <>
class HDF5_writer<VECTOR_DIST>
{
public:

	template<unsigned int dim, typename St, typename prp>
	inline void save(const std::string & filename,
			         const openfpm::vector<Point<dim,St>> & v_pos,
					 const openfpm::vector<prp> & v_prp) const
	{
		Vcluster & v_cl = create_vcluster();

		//Pack_request vector
		size_t req = 0;

		//std::cout << "V_pos.size() before save: " << v_pos.size() << std::endl;

		//Pack request
		Packer<typename std::remove_reference<decltype(v_pos)>::type,HeapMemory>::packRequest(v_pos,req);
		Packer<typename std::remove_reference<decltype(v_prp)>::type,HeapMemory>::packRequest(v_prp,req);

		// allocate the memory
		HeapMemory pmem;
		//pmem.allocate(req);
		ExtPreAlloc<HeapMemory> & mem = *(new ExtPreAlloc<HeapMemory>(req,pmem));
		mem.incRef();

		//Packing

		Pack_stat sts;

		Packer<typename std::remove_reference<decltype(v_pos)>::type,HeapMemory>::pack(mem,v_pos,sts);
		Packer<typename std::remove_reference<decltype(v_prp)>::type,HeapMemory>::pack(mem,v_prp,sts);

		/*****************************************************************
		 * Create a new file with default creation and access properties.*
		 * Then create a dataset and write data to it and close the file *
		 * and dataset.                                                  *
		 *****************************************************************/

		int mpi_rank = v_cl.getProcessUnitID();
		int mpi_size = v_cl.getProcessingUnits();

		MPI_Comm comm = v_cl.getMPIComm();
		MPI_Info info  = MPI_INFO_NULL;

		// Set up file access property list with parallel I/O access

		hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
		H5Pset_fapl_mpio(plist_id, comm, info);

		// Create a new file collectively and release property list identifier.
		hid_t file = H5Fcreate (filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
		H5Pclose(plist_id);

		size_t sz = pmem.size();
		//std::cout << "Pmem.size: " << pmem.size() << std::endl;
		openfpm::vector<size_t> sz_others;
		v_cl.allGather(sz,sz_others);
		v_cl.execute();

		size_t sum = 0;

		for (size_t i = 0; i < sz_others.size(); i++)
			sum += sz_others.get(i);

		//Size for data space in file
		hsize_t fdim[1] = {sum};

		//Size for data space in file
		hsize_t fdim2[1] = {(size_t)mpi_size};

		//Create data space in file
		hid_t file_dataspace_id = H5Screate_simple(1, fdim, NULL);

		//Create data space in file
		hid_t file_dataspace_id_2 = H5Screate_simple(1, fdim2, NULL);

		//Size for data space in memory
		hsize_t mdim[1] = {pmem.size()};

		//Create data space in memory
		hid_t mem_dataspace_id = H5Screate_simple(1, mdim, NULL);

		//Create data set in file
		hid_t file_dataset = H5Dcreate (file, "vector_dist", H5T_NATIVE_CHAR, file_dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

		//Create data set 2 in file
		hid_t file_dataset_2 = H5Dcreate (file, "metadata", H5T_NATIVE_INT, file_dataspace_id_2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

		//H5Pclose(plist_id);
		H5Sclose(file_dataspace_id);
		H5Sclose(file_dataspace_id_2);

		hsize_t block[1] = {pmem.size()};

		//hsize_t stride[1] = {1};

		hsize_t count[1] = {1};

		hsize_t offset[1] = {0};

		for (int i = 0; i < mpi_rank; i++)
		{
			if (mpi_rank == 0)
				offset[0] = 0;
			else
				offset[0] += sz_others.get(i);
		}

		int metadata[mpi_size];

		for (int i = 0; i < mpi_size; i++)
			metadata[i] = sz_others.get(i);

		//Select hyperslab in the file.
		file_dataspace_id = H5Dget_space(file_dataset);
		H5Sselect_hyperslab(file_dataspace_id, H5S_SELECT_SET, offset, NULL, count, block);

		file_dataspace_id_2 = H5Dget_space(file_dataset_2);


		//Create property list for collective dataset write.
		plist_id = H5Pcreate(H5P_DATASET_XFER);
		H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

		//Write a data set to a file
		H5Dwrite(file_dataset, H5T_NATIVE_CHAR, mem_dataspace_id, file_dataspace_id, plist_id, (const char *)pmem.getPointer());

		//Write a data set 2 to a file
		H5Dwrite(file_dataset_2, H5T_NATIVE_INT, H5S_ALL, file_dataspace_id_2, plist_id, metadata);


		//Close/release resources.
		H5Dclose(file_dataset);
		H5Sclose(file_dataspace_id);
		H5Dclose(file_dataset_2);
		H5Sclose(file_dataspace_id_2);
		H5Sclose(mem_dataspace_id);
		H5Pclose(plist_id);
		H5Fclose(file);
	}

};


#endif /* OPENFPM_IO_SRC_HDF5_WR_HDF5_WRITER_VD_HPP_ */
