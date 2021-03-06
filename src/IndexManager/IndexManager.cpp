#include "IndexManager.h"

IndexManager::IndexManager(BufferManager* bm) {
	this->bm = bm;
}

IndexManager::~IndexManager() {

}
void IndexManager::newFile(std::string filename) {
	std::fstream file;
	file.open(filename, std::ios::out);
}
bool IndexManager::is_tree_already_exist(std::string filename, int type) {
	if (type == 0) {
		if (int_index.count(filename) == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	else if (type == -1) {
		if (float_index.count(filename) == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	else if (type > 0) {
		if (string_index.count(filename) == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	else {
		cout << "unknown type" << endl;
		return true;
	}
}
int IndexManager::get_degree(int type) {
	if (type == 0)
	{
		return (BLOCK_SIZE - (sizeof(char) + 3 * sizeof(int))) / (2 * sizeof(int)) - 2;
	}
	else if (type == -1)
	{
		return (BLOCK_SIZE - (sizeof(char) + 3 * sizeof(int))) / (sizeof(float) + sizeof(int)) - 2;
	}
	else if (type > 0)
	{
		return (BLOCK_SIZE - (sizeof(char) + 3 * sizeof(int))) / (type + sizeof(int)) - 2;
	}
	else
	{
		cout << "unknown type" << endl;
	}
}
void IndexManager::create_index(const std::string &db_name, const std::string &table_name, const std::string &index_name, int type) {
	std::string filename = table_name;
	newFile(filename);
	BlockInfo bpt = bm->get_block_info(db_name, table_name, 1, 0);//0 block for tree info
	char* block = bpt->cBlock;
	*((int*)block) = -1;
	*((int*)(block + sizeof(int))) = -1;
	if (is_tree_already_exist(filename, type)) {
		cout<<"Error: a bplustree is already existed"<<endl;
	}
	else {
		if (type == 0) {//int
			int_index[filename] = std::make_shared<BPlusTree<int>>(bm, db_name,table_name,get_degree(type),sizeof(int));
		}
		else if (type == -1) {//float
			float_index[filename] = std::make_shared<BPlusTree<float>>(bm, db_name, table_name, get_degree(type), sizeof(float));
		}
		else if (type > 0) {
			string_index[filename] = std::make_shared<BPlusTree<std::string>>(bm, db_name, table_name, get_degree(type), type);
			if (string_index_length.count(filename) == 0)
			{
				string_index_length[filename] = type;
			}
		}
	}
}

void IndexManager::readintree(std::string db_name,std::string table_name, int type) {
	std::string filename = table_name;
	char* block = bm->get_block_info(db_name, table_name, 1, 0)->cBlock;
	BlockId Root = *((int*)(block));
	BlockId LeftLeaf = *((int*)(block + sizeof(int)));
	if (type == 0) {
		
		int_index[filename] = std::make_shared<BPlusTree<int>>(bm, db_name, table_name, get_degree(type), sizeof(int), Root, LeftLeaf);
	}
	else if (type == -1) {//float
		float_index[filename] = std::make_shared<BPlusTree<float>>(bm, db_name, table_name, get_degree(type), sizeof(float), Root, LeftLeaf);
	}
	else if (type > 0) {
		string_index[filename] = std::make_shared<BPlusTree<std::string>>(bm, db_name, table_name, get_degree(type), type, Root, LeftLeaf);
		if (string_index_length.count(filename) == 0)
		{
			string_index_length[filename] = type;
		}
	}
}

bool IndexManager::find_element(const std::string &db_name, const std::string &table_name,
	const std::string &index_name, const std::string &data, std::vector<int> &block_id,int type) {
	std::string filename = table_name;
	if (!is_tree_already_exist(filename, type))
	{
		//从硬盘中加载
		readintree(db_name, table_name, type);
	}
	if (type == 0) {
		block_id.push_back(int_index[filename]->Search(atoi(data.c_str())));
		if (block_id[0] == -1)return false;
		return true;
	}
	else if (type == -1) {
		block_id.push_back(float_index[filename]->Search(atof(data.c_str())));
		if (block_id[0] == -1)return false;
		return true;
	}
	else if (type > 0) {
		block_id.push_back(string_index[filename]->Search(data));
		if (block_id[0] == -1)return false;
		return true;
	}

	return false;
}
bool IndexManager::insert_index(const std::string &db_name, const std::string &table_name,
	const std::string &index_name, const std::string &data, int block_id, int type) {
	std::string filename = table_name;
	if (!is_tree_already_exist(filename, type))
	{
		//从硬盘中加载
		readintree(db_name, table_name, type);
	}
	if (type == 0) {
		return int_index[filename]->InsertElement(atoi(data.c_str()), block_id);
	}
	else if (type == -1) {
		return float_index[filename]->InsertElement(atof(data.c_str()),block_id);
	}
	else if (type > 0) {
		return string_index[filename]->InsertElement(data,block_id);
	}


	return false;
}
bool IndexManager::delete_index(const std::string &db_name, const std::string &table_name,
	const std::string &index_name, const std::string &data, int type) {
	std::string filename = table_name;
	if (!is_tree_already_exist(filename, type))
	{
		//从硬盘中加载
		readintree(db_name, table_name, type);
	}
	if (type == 0) {
		return int_index[filename]->DeleteElement(atoi(data.c_str()));
	}
	else if (type == -1) {
		return float_index[filename]->DeleteElement(atof(data.c_str()));
	}
	else if (type > 0) {
		return string_index[filename]->DeleteElement(data);
	}
	return false;
}
void IndexManager::drop_index(const std::string &table_name, const std::string &index_name, int type) {
	std::string filename = table_name;
	if (!is_tree_already_exist(filename, type))
	{
		if (type == 0)
		{
			int_index.erase(filename);
		}
		else if (type == -1)
		{
			float_index.erase(filename);
		}
		// 字符串
		else if (type > 0)
		{
			string_index.erase(filename);
			string_index_length.erase(filename);
		}
	}
	fstream _file;
	_file.open(filename, ios::in);
	if (_file) {
		remove(filename.c_str());
	}
}