#include <stdlib.h>

#include <fs_info.h>

#include "vmwfs.h"

VMWNode* root_node;

status_t
vmwfs_mount(fs_volume *_vol, const char *device, uint32 flags, const char *args, ino_t *_rootID)
{
	if (device != NULL)
		return B_BAD_VALUE;
	
	shared_folders = new VMWSharedFolders();
	if (shared_folders->InitCheck() != B_OK) {
		delete shared_folders;
		return B_ERROR;
	}
	
	root_node = new VMWNode("", NULL);	
	
	if (root_node == NULL)
		return B_NO_MEMORY;
	
	*_rootID = root_node->GetInode();
	
	_vol->private_volume = root_node;
	_vol->ops = &volume_ops;
	
	status_t ret = publish_vnode(_vol, *_rootID, (void*)_vol->private_volume,
			&vnode_ops, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH | S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH, 0);

	return ret;
}

status_t
vmwfs_unmount(fs_volume* volume)
{
	delete root_node;
	delete shared_folders;
	
	return B_OK;
}

status_t
vmwfs_read_fs_info(fs_volume* volume, struct fs_info* info)
{
	info->flags = B_FS_IS_PERSISTENT;
	info->block_size = FAKE_BLOCK_SIZE;
	info->io_size = 4096;
	info->total_blocks = 2 * 1024 * 1024 * 1024 * 1024 / info->block_size; // 2GB0
	info->free_blocks = info->total_blocks;
	info->total_nodes = info->block_size;
	info->free_nodes = info->block_size;
	
	strcpy(info->device_name, "");
	strcpy(info->volume_name, "VMW Shared Folders");
	
	return B_OK;
}

status_t
vmwfs_write_fs_info(fs_volume* volume, const struct fs_info* info, uint32 mask)
{
	// TODO : Store volume name ?
	return B_OK;
}

status_t
vmwfs_get_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags, bool reenter)
{
	vnode->private_node = NULL;
	vnode->ops = &vnode_ops;
	_flags = 0;
	
	VMWNode* node = root_node->GetChild(id);
	
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;
	
	vnode->private_node = node;
	
	char* path = node->GetPath();
	if (path == NULL)
		return B_NO_MEMORY;
	
	vmw_attributes attributes;
	bool is_dir;
	status_t ret = shared_folders->GetAttributes(path, &attributes, &is_dir);
	free(path);
	if (ret != B_OK)
		return ret;
	
	*_type = 0;
	*_type |= (CAN_READ(attributes) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
	*_type |= (CAN_WRITE(attributes) ? S_IWUSR : 0);
	*_type |= (CAN_EXEC(attributes) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
	*_type |= (is_dir ? S_IFDIR : S_IFREG);	
	
	return B_OK;
}
