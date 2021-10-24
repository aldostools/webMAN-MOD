#!/usr/bin/env python
from __future__ import with_statement
from Struct2 import Struct
from fself import SelfHeader, AppInfo

import struct
import sys
import hashlib
import os
import getopt
import ConfigParser
import io
import glob
import re
import sys
if sys.platform.startswith('win'):
	import msvcrt

TYPE_NPDRMSELF = 0x1
TYPE_RAW = 0x3
TYPE_DIRECTORY = 0x4

TYPE_OVERWRITE_ALLOWED = 0x80000000

MaxCacheSize = 0x8000000

debug = False

this = sys.modules[__name__]
this.dirCount = 0

this.category = ""
this.contentType = 0x9
this.prefixPath = "../../"
this.prefixLen = 6

class MetaHeader(Struct):
	__endian__ = Struct.BE
	def __format__(self):
		self.unk1 	= Struct.uint32
		self.unk2 	= Struct.uint32
		self.drmType 	= Struct.uint32
		self.unk4 	= Struct.uint32

		self.unk21 	= Struct.uint32
		self.contentType = Struct.uint32
		self.unk23 	= Struct.uint32
		self.unk24 	= Struct.uint32

		self.packageType = Struct.uint32
		self.unk32 	= Struct.uint32
		self.unk33 	= Struct.uint32
		self.secondaryVersion 	= Struct.uint16
		self.unk34 	= Struct.uint16

		self.dataSize 	= Struct.float
		self.unk42 	= Struct.uint32
		self.unk43 	= Struct.uint32
		self.packagedBy 	= Struct.uint16
		self.packageVersion 	= Struct.uint16

class DigestBlock(Struct):
	__endian__ = Struct.BE
	def __format__(self):
		self.type 	= Struct.uint32
		self.size 	= Struct.uint32
		self.isNext = Struct.uint64

class FileHeader(Struct):
	__endian__ = Struct.BE
	def __format__(self):
		self.fileNameOff 	= Struct.uint32
		self.fileNameLength = Struct.uint32
		self.fileOff 		= Struct.uint64
		self.fileSize		= Struct.uint64
		self.flags			= Struct.uint32
		self.padding		= Struct.uint32
	def __str__(self):
		out  = ""
		out += "[X] File Name: %s [" % self.fileName
		if self.flags & 0xFF == TYPE_NPDRMSELF:
			out += "NPDRM Self]"
		elif self.flags & 0xFF == TYPE_DIRECTORY:
			out += "Directory]"
		elif self.flags & 0xFF == TYPE_RAW:
			out += "Raw Data]"
		else:
			out += "Unknown]"
		if (self.flags & TYPE_OVERWRITE_ALLOWED ) != 0:
			out += " Overwrite allowed.\n"
		else:
			out += " Overwrite NOT allowed.\n"
		out += "\n"

		out += "[X] File Name offset: %08x\n" % self.fileNameOff
		out += "[X] File Name Length: %08x\n" % self.fileNameLength
		out += "[X] Offset To File Data: %016x\n" % self.fileOff

		out += "[X] File Size: %016x" % self.fileSize + " ({:,} bytes)\n".format(self.fileSize)
		out += "[X] Flags: %08x\n" % self.flags
		out += "[X] Padding: %08x\n\n" % self.padding
		assert self.padding == 0, "I guess I was wrong, this is not padding."

		return out

	def __repr__(self):
		return self.fileName + ("<FileHeader> Size: 0x%016x" % self.fileSize)
	def __init__(self):
		Struct.__init__(self)
		self.fileName = ""
	def doWork(self, decrypteddata, context = None):
		if context == None:
			self.fileName = nullterm(decrypteddata[self.fileNameOff:self.fileNameOff+self.fileNameLength])
		else:
			self.fileName = nullterm(crypt(listToString(context), decrypteddata[self.fileNameOff:self.fileNameOff+self.fileNameLength], self.fileNameLength))
	def dump(self, directory, data, header):
		self.fileName = self.fileName.replace("../../", "")
		if self.flags & 0xFF == TYPE_DIRECTORY:
			try:
				os.makedirs(directory + "/" + self.fileName)
			except Exception, e:
				print

		else:
			tFile = open(directory + "/" + self.fileName, "wb")
			tFile.write(data[self.fileOff:self.fileOff+self.fileSize])

class Header(Struct):
	__endian__ = Struct.BE
	def __format__(self):
		self.magic		 = Struct.uint32
		self.type		 = Struct.uint32
		self.pkgInfoOff	 = Struct.uint32
		self.unk1		 = Struct.uint32

		self.headSize	 = Struct.uint32
		self.itemCount	 = Struct.uint32
		self.packageSize = Struct.uint64

		self.dataOff	 = Struct.uint64
		self.dataSize	 = Struct.uint64

		self.contentID	 = Struct.uint8[0x30]
		self.QADigest	 = Struct.uint8[0x10]
		self.KLicensee	 = Struct.uint8[0x10]

	def __str__(self):
		context = keyToContext(self.QADigest)
		setContextNum(context, 0xFFFFFFFFFFFFFFFF)
		licensee = crypt(listToString(context), listToString(self.KLicensee), 0x10)

		out  = ""
		out += "[X] Magic: %08x\n" % self.magic
		out += "[X] Type: %08x\n" % self.type
		out += "[X] Header size: %08x\n" % self.headSize
		out += "[X] Content Type: %08x %s\n" % (this.contentType, this.category)
		if this.dirCount > 0:
			out += "[X] Item count: " + "{:,}".format(self.itemCount) + " ({:,} files".format(self.itemCount - this.dirCount) + ", {:,} directories)".format(this.dirCount) + " [0x{:08x}]\n".format(self.itemCount)
		else:
			out += "[X] Item count: " + "{:,}".format(self.itemCount) + " [0x{:08x}]\n".format(self.itemCount)
		out += "[X] Package size: " + "{:,} bytes".format(self.packageSize) + " -> {:,.1f} MB".format(float(self.packageSize)/1024/1024) + " [0x{:016x}]\n".format(self.packageSize)
		out += "[X] Data offset: %016x\n" % self.dataOff
		out += "[X] Data size: " + "{:016x}\n".format(self.dataSize)
		out += "[X] Content ID: %s\n" % (nullterm(self.contentID))
		out += "[X] QA_Digest: %s\n" % (nullterm(self.QADigest, True))
		out += "[X] Package information offset: %08x\n" % self.pkgInfoOff
		return out

def wait():
	if sys.platform.startswith('win'):
		msvcrt.getch()

def listToString(inlist):
	if isinstance(inlist, list):
		return ''.join(["%c" % el for el in inlist])
	else:
		return ""

def nullterm(str_plus, printhex=False):
	if isinstance(str_plus, list):
		if printhex:
			str_plus = ''.join(["%X" % el for el in str_plus])
		else:
			str_plus = listToString(str_plus)
	z = str_plus.find('\0')
	if z != -1:
		return str_plus[:z]
	else:
		return str_plus

def keyToContext(key):
	if isinstance(key, list):
		key = listToString(key)
		key = key[0:16]
	largekey = []
	for i in range(0, 8):
		largekey.append(ord(key[i]))
	for i in range(0, 8):
		largekey.append(ord(key[i]))
	for i in range(0, 8):
		largekey.append(ord(key[i+8]))
	for i in range(0, 8):
		largekey.append(ord(key[i+8]))
	for i in range(0, 0x20):
		largekey.append(0)
	return largekey

# Thanks to anonymous for the help with the RE of this part,
# the x86 mess of ands and ors made my head go BOOM headshot.
def manipulate(key):
	if not isinstance(key, list):
		return
	tmp = listToString(key[0x38:])

	tmpnum = struct.unpack('>Q', tmp)[0]
	tmpnum += 1
	tmpnum = tmpnum & 0xFFFFFFFFFFFFFFFF
	setContextNum(key, tmpnum)

def setContextNum(key, tmpnum):
	tmpchrs = struct.pack('>Q', tmpnum)

	key[0x38] = ord(tmpchrs[0])
	key[0x39] = ord(tmpchrs[1])
	key[0x3a] = ord(tmpchrs[2])
	key[0x3b] = ord(tmpchrs[3])
	key[0x3c] = ord(tmpchrs[4])
	key[0x3d] = ord(tmpchrs[5])
	key[0x3e] = ord(tmpchrs[6])
	key[0x3f] = ord(tmpchrs[7])

import pkgcrypt

def crypt(key, inbuf, length):
	return pkgcrypt.pkgcrypt(key, inbuf, length);

	# Original python (slow) implementation
	ret = ""
	offset = 0
	while length > 0:
		bytes_to_dump = length
		if length > 0x10:
			bytes_to_dump = 0x10
		outhash = SHA1(listToString(key)[0:0x40])
		for i in range(0, bytes_to_dump):
			ret += chr(ord(outhash[i]) ^ ord(inbuf[offset]))
			offset += 1
		manipulate(key)
		length -= bytes_to_dump
	return ret

def SHA1(data):
	m = hashlib.sha1()
	m.update(data)
	return m.digest()

pkgcrypt.register_sha1_callback(SHA1)

def listPkg(filename):
	with open(filename, 'rb') as fp:
		header = Header()
		data = fp.read(len(header))
		header.unpack(data[:len(header)])
		data += fp.read(0x200 * header.itemCount)
	fp.close()

	print 'Listing "' + filename + '"'
	print
	print header
	if header.type == 0x80000001:
		print "ERROR: Unsupported Package Type 0x%x (finished /  signed pkg)" % 0x80000001
		wait()
		return

	assert (header.type == 0x00000001) , 'Unsupported Type'
	if header.itemCount > 0:
		print "+) overwrite, -) no overwrite"
		print
		context = keyToContext(header.QADigest)
		dataEnc = data[header.dataOff:header.dataOff + 0x200 * header.itemCount]
		decData = crypt(listToString(context), dataEnc, 0x200 * header.itemCount)
		totalSize = 0

		for i in range(0, header.itemCount):
			fileD = FileHeader()
			fileD.unpack(decData[0x20 * i:0x20 * i + 0x20])
			fileD.doWork(decData)
			out = ""
			if fileD.flags & 0xFF == TYPE_NPDRMSELF:
				out += " NPDRM SELF:"
			elif fileD.flags & 0xFF == TYPE_DIRECTORY:
				out += "  directory:"
				this.dirCount += 1
			elif fileD.flags & 0xFF == TYPE_RAW:
				out += "   raw data:"
			else:
				out += "    unknown:"
			if (fileD.flags & TYPE_OVERWRITE_ALLOWED ) != 0:
				out += "+"
			else:
				out += "-"

			if fileD.flags & 0xFF == TYPE_DIRECTORY:
				out += "      <DIR>  "
			else:
				out += "%11d  " % fileD.fileSize

			out += fileD.fileName.replace("../../", "")
			print out,
			print
			totalSize += fileD.fileSize

		print
		print ">> Item count: " + "{:,}".format(header.itemCount) + " ({:,} files".format(header.itemCount - this.dirCount) + ", {:,} directories) ".format(this.dirCount) + "{:,} bytes".format(totalSize) + " -> {:,.1f} MB".format(float(totalSize)/1024/1024)

def unpack(filename):
	with open(filename, 'rb') as fp:
		header = Header()
		data = fp.read(len(header))
		header.unpack(data[:len(header)])
		data += fp.read(0x200 * header.itemCount)
	fp.close()

	print 'Extracting "' + filename + '"'
	print header
	if header.type == 0x80000001:
		print "ERROR: Unsupported Package Type 0x%x (finished /  signed pkg)" % 0x80000001
		wait()
		return

	assert header.type == (0x00000001), 'Unsupported Type'
	if header.itemCount > 0:
		context = listToString(keyToContext(header.QADigest))
		dataEnc = data[header.dataOff:header.dataOff + (0x200 * header.itemCount)]
		decData = crypt(context, dataEnc, len(dataEnc))
		directory = nullterm(header.contentID)
		try:
			os.makedirs(directory)
		except Exception, e:
			pass

		# Decrypt whole PKG in chunks
		context = listToString(keyToContext(header.QADigest))
		fp = open(filename, 'rb')
		fp.seek(header.dataOff)
		fileSize = header.packageSize
		UseCacheFile = (fileSize > MaxCacheSize)
		if UseCacheFile:
			# Create cache file with decoded
			decFile = open(filename + ".dec", 'wb')
			chunkSize = MaxCacheSize
			while fileSize > 0:
				pkgData = fp.read(chunkSize)
				decFile.write(crypt(context, pkgData, chunkSize))
				fileSize -= chunkSize
				if fileSize < chunkSize:
					chunkSize = fileSize
			decFile.close()
			fp.close()
		else:
			pkgData = fp.read(fileSize)
			fileData = crypt(context, pkgData, len(pkgData))
			fp.close()

		totalSize = 0

		if UseCacheFile:
			decFile = open(filename + ".dec", 'rb')

		for i in range(0, header.itemCount):
			fileD = FileHeader()
			fileD.unpack(decData[0x20 * i:0x20 * i + 0x20])
			fileD.doWork(decData)
			fileD.fileName = fileD.fileName.replace("../../", "")
			print '%s' % fileD.fileName

			if fileD.flags & 0xFF == TYPE_DIRECTORY:
				this.dirCount += 1
				try:
					os.makedirs(directory + "/" + fileD.fileName)
				except Exception, e:
					pass
			else:
				fileSize = fileD.fileSize
				totalSize += fileSize

				# Create output file
				outFile = open(directory + "/" + fileD.fileName, "wb")
				if UseCacheFile:
					decFile.seek(fileD.fileOff)
					if fileSize > MaxCacheSize:
						chunkSize = MaxCacheSize
						while fileSize > 0:
							fileData = decFile.read(chunkSize)
							if not fileData:
								break
							outFile.write(fileData)
							fileSize -= chunkSize
							if fileSize < chunkSize:
								chunkSize = fileSize
					else:
						fileData = decFile.read(fileSize)
						outFile.write(fileData)
				else:
					outFile.write(fileData[fileD.fileOff:fileD.fileOff + fileSize])
				outFile.close()

		if UseCacheFile:
			decFile.close()
			os.remove(filename + ".dec")

	print
	print ">> Item count: " + "{:,}".format(header.itemCount) + " ({:,} files".format(header.itemCount - this.dirCount) + ", {:,} directories) ".format(this.dirCount) + "{:,} bytes".format(totalSize) + " -> {:,.1f} MB".format(float(totalSize)/1024/1024)

def getFiles(files, folder, original_len):
	oldfolder = folder
	#foundFiles = glob.glob( os.path.join(folder, '*') )
	# Fix brackets in directory names
	foundFiles = glob.glob( os.path.join(folder.replace('[', '[[]'), '*') )
	filesList = []
	for filepath in foundFiles:
		if not os.path.isdir(filepath):
			filesList.append(filepath)
		else:
			newpath = filepath[original_len:].replace("\\", "/")
			folder = FileHeader()
			folder.fileName = newpath
			folder.fileNameOff 	= 0
			folder.fileNameLength = len(folder.fileName)
			folder.fileOff 		= 0
			folder.fileSize 	= 0
			folder.flags		= TYPE_OVERWRITE_ALLOWED | TYPE_DIRECTORY
			folder.padding 		= 0
			files.append(folder)
			getFiles(files, filepath, original_len)

	for filepath in filesList:
			newpath = filepath[original_len:].replace("\\", "/")
			file = FileHeader()
			file.fileName = newpath
			file.fileNameOff 	= 0
			file.fileNameLength = len(file.fileName)
			file.fileOff 		= 0
			file.fileSize 	= os.path.getsize(filepath)
			file.flags		= TYPE_OVERWRITE_ALLOWED | TYPE_RAW
			if newpath == "USRDIR/EBOOT.BIN":
				file.fileSize = ((file.fileSize - 0x30 + 63) & ~63) + 0x30
				file.flags		= TYPE_OVERWRITE_ALLOWED | TYPE_NPDRMSELF
			file.padding 	= 0
			files.append(file)

def pack(folder, contentid, outname=None):

	folder = os.path.normpath(folder) + "/"

	if os.path.exists(folder + "PARAM.SFO"):
		this.prefixPath = "" # do not use ../../ on standard packages
		this.prefixLen = 0
		if os.path.exists(folder + "USRDIR/EBOOT.BIN"):
			this.contentType = 0x05 # gameExec
		else:
			this.contentType = 0x04 # gameData
		with open(folder + "PARAM.SFO", 'rb') as fp:
			sfo = fp.read(0x1000)
			if sfo.find('\x00DG\x00') > 0:
				this.category = "(CATEGORY = DG - Disc Game)"
			elif sfo.find('\x00HG\x00') > 0:
				this.category = "(CATEGORY = HG - HDD Game)"
			elif sfo.find('\x00GD\x00') > 0:
				this.category = "(CATEGORY = GD - Game Data)"
			elif sfo.find('\x00HM\x00') > 0:
				this.category = "(CATEGORY = HM - PlayStation Network)"
			elif sfo.find('\x00SF\x00') > 0:
				this.category = "(CATEGORY = SF - PlayStation Store)"
			elif sfo.find('\x00CB\x00') > 0:
				this.category = "(CATEGORY = CB - Network)"
			elif sfo.find('\x00AT\x00') > 0:
				this.category = "(CATEGORY = AT - App TV)"
			elif sfo.find('\x00AS\x00') > 0:
				this.category = "(CATEGORY = AS - App Streaming)"
			elif sfo.find('\x00AV\x00') > 0:
				this.category = "(CATEGORY = AV - App Video)"
			elif sfo.find('\x00BV\x00') > 0:
				this.category = "(CATEGORY = BV - App Video)"
			elif sfo.find('\x00AM\x00') > 0:
				this.category = "(CATEGORY = AM - App Music)"
			elif sfo.find('\x00AP\x00') > 0:
				this.category = "(CATEGORY = AP - App Photo)"
			elif sfo.find('\x00WT\x00') > 0:
				this.category = "(CATEGORY = WT - Web TV)"
				this.contentType = 0x19 # Web TV
			elif sfo.find('\x00SD\x00') > 0:
				this.category = "(CATEGORY = SD - Save Data)"
			elif sfo.find('\x001P\x00') > 0:
				this.category = "(CATEGORY = 1P - PS1 Classic)"
				this.contentType = 0x06 # PS1 Classic
			elif sfo.find('\x002P\x00') > 0:
				this.category = "(CATEGORY = 2P - PS2 Classic)"
				this.contentType = 0x12 # PS2 Classic
			elif sfo.find('\x00MN\x00') > 0:
				this.category = "(CATEGORY = MN - PSP Mini)"
				this.contentType = 0x0F # PSP Mini
			elif sfo.find('\x00PE\x00') > 0:
				this.category = "(CATEGORY = PE - PSP Remaster)"
				this.contentType = 0x14 # PSP Remaster
			elif sfo.find('\x00TR\x00') > 0:
				this.category = "(CATEGORY = TR - Theme)"
				this.contentType = 0x09 # Theme
		fp.close()
	elif contentid.find("VSHMODULE") >= 0:
		this.prefixPath = "" # do not use ../../ on standard packages
		this.prefixLen = 0
		this.contentType = 0x0C # vsh module
		this.category = "(VSH MODULE)"

	qadigest = hashlib.sha1()

	header = Header()
	header.magic = 0x7F504B47
	header.type = 0x01
	header.pkgInfoOff = 0xC0
	header.unk1 = 0x05

	header.headSize = 0x80
	header.itemCount = 0
	header.packageSize = 0

	header.dataOff = 0x140
	header.dataSize = 0

	for i in range(0, 0x30):
		header.contentID[i] = 0

	for i in range(0,0x10):
		header.QADigest[i] = 0
		header.KLicensee[i] = 0

	#content type	type name				install path (on ps3)	notes
	#0x00000004		GameData (also Patches)	/dev_hdd0/game/
	#0x00000005		Game_Exec				/dev_hdd0/game/
	#0x00000006		PS1emu					/dev_hdd0/game/
	#0x00000007		PSP & PCEngine			/dev_hdd0/game/
	#0x00000008
	#0x00000009		Theme					/dev_hdd0/theme
	#0x0000000A		Widget					/dev_hdd0/widget
	#0x0000000B		License					/dev_hdd0/home/<current user>/exdata
	#0x0000000C		VSH Module				/dev_hdd0/vsh/modules/
	#0x0000000D		PSN Avatar				/dev_hdd0/home/<current user>/psn_avatar
	#0x0000000E		PSPgo					/dev_hdd0/game/	Displayed as Unknown Album: Corrupted Data
	#0x0000000F		Minis					/dev_hdd0/game/
	#0x00000010		NEOGEO					/dev_hdd0/game/
	#0x00000011		VMC						/dev_hdd0/tmp/vmc/
	#0x00000012		Seen on PS2 classic		/dev_hdd0/game/
	#0x00000014		Seen on PSP remastered	/dev_hdd0/game/
	#0x00000015		PSVita (PSP2GD)
	#0x00000016		PSVita (PSP2AC)
	#0x00000017		PSVita (PSP2LA)
	#0x00000019		WT (Web TV?)			/dev_hdd0/game/

	metaBlock = MetaHeader()
	metaBlock.unk1 		= 1 #doesnt change output of --extract
	metaBlock.unk2 		= 4 #doesnt change output of --extract
	metaBlock.drmType 	= 3 #1 = Network, 2 = Local, 3 = Free, anything else = unknown
	metaBlock.unk4 		= 2

	metaBlock.unk21 	= 4
	metaBlock.contentType = this.contentType #9 == Theme, 5 == gameexec, 4 == gamedata
	metaBlock.unk23 	= 3
	metaBlock.unk24 	= 4

	metaBlock.packageType = 0xE #packageType 0x10 == patch, 0x8 == Demo&Key, 0x0 == Demo&Key (AND UserFiles = NotOverWrite), 0xE == normal, use 0xE for gamexec, and 8 for gamedata
	metaBlock.unk32 	= 4   #when this is 5 secondary version gets used??
	metaBlock.unk33 	= 8   #doesnt change output of --extract
	metaBlock.secondaryVersion = 0
	metaBlock.unk34 	= 0

	metaBlock.dataSize 	= 0
	metaBlock.unk42 	= 5
	metaBlock.unk43 	= 4
	metaBlock.packagedBy = 0x1061
	metaBlock.packageVersion = 0

	files = []
	getFiles(files, folder, len(folder))
	header.itemCount = len(files)
	dataToEncrypt = ""
	fileDescLength = 0
	fileOff = 0x20 * len(files)
	for file in files:
		file.fileNameLength += this.prefixLen if (file.fileName[:4] != "dev_" or this.contentType == 0x09) else 9
		alignedSize = (file.fileNameLength + 0x0F) & ~0x0F
		file.fileNameOff = fileOff
		fileOff += alignedSize
	for file in files:
		file.fileOff = fileOff
		fileOff += (file.fileSize + 0x0F) & ~0x0F
		dataToEncrypt += file.pack()
	for file in files:
		alignedSize = (file.fileNameLength + 0x0F) & ~0x0F
		dataToEncrypt += this.prefixPath + file.fileName if (file.fileName[:4] != "dev_" or this.contentType == 0x09) else "../../../" + file.fileName
		dataToEncrypt += "\0" * (alignedSize-file.fileNameLength)

	fileDescLength = len(dataToEncrypt)
	firstEncData = dataToEncrypt
	qadigest.update(firstEncData)

	encDataLen = fileDescLength
	for file in files:
		if not file.flags & 0xFF == TYPE_DIRECTORY:
			#path = os.path.join(folder, file.fileName)
			#fileDataLen = os.path.getsize(path)
			#fp = open(path, 'rb')
			#while False:
			#	# Process data in 100MB chunks
			#	fileData = fp.read(0x5F5E100)
			#	if not fileData:
			#		break
			#	qadigest.update(fileData)
			#	encDataLen += len(fileData)
			#fp.close()
			#encDataLen += len("\0" * (((file.fileSize + 0x0F) & ~0x0F))) # - fileDataLen))
			encDataLen += ((file.fileSize + 0x0F) & ~0x0F)
		else:
			this.dirCount += 1

	header.dataSize = encDataLen
	metaBlock.dataSize = header.dataSize
	header.packageSize = header.dataSize + 0x1A0
	head = header.pack()
	qadigest.update(head)
	qadigest.update(dataToEncrypt[0:fileDescLength])
	QA_Digest = qadigest.digest()

	for i in range(0, 0x10):
		header.QADigest[i] = ord(QA_Digest[i])

	for i in range(0, min(len(contentid), 0x30)):
		header.contentID[i] = ord(contentid[i])

	context = keyToContext(header.QADigest)
	setContextNum(context, 0xFFFFFFFFFFFFFFFF)
	licensee = crypt(listToString(context), listToString(header.KLicensee), 0x10)

	for i in range(0, min(len(contentid), 0x10)):
		header.KLicensee[i] = ord(licensee[i])

	if outname == None:
		outname = contentid + ".pkg"

	print 'Creating ' + outname
	print

	outFile = open(outname, 'wb')
	outFile.write(header.pack())
	headerSHA = SHA1(header.pack())[3:19]
	outFile.write(headerSHA)

	metaData = metaBlock.pack()
	metaBlockSHA = SHA1(metaData)[3:19]
	metaBlockSHAPad = '\0' * 0x30

	context = keyToContext([ord(c) for c in metaBlockSHA])
	metaBlockSHAPadEnc = crypt(listToString(context), metaBlockSHAPad, 0x30)

	context = keyToContext([ord(c) for c in headerSHA])
	metaBlockSHAPadEnc2 = crypt(listToString(context), metaBlockSHAPadEnc, 0x30)
	outFile.write(metaBlockSHAPadEnc2)
	outFile.write(metaData)
	outFile.write(metaBlockSHA)
	outFile.write(metaBlockSHAPadEnc)

	context = listToString(keyToContext(header.QADigest))

	#os.system('cls')

	if firstEncData != 0:
		outFile.write(crypt(context, firstEncData, len(firstEncData)))
		firstEncData = 0

	# Process/encrypt file data in chunks
	for file in files:
		if not file.flags & 0xFF == TYPE_DIRECTORY:
			print(file.fileName)
			path = os.path.join(folder, file.fileName)
			fp = open(path, 'rb')

			fileSize = file.fileSize
			if fileSize > MaxCacheSize:
				chunkSize = MaxCacheSize
				while fileSize > 0:
					fileData = fp.read(chunkSize)
					outFile.write(crypt(context, fileData, chunkSize))
					fileSize -= chunkSize
					if fileSize < chunkSize:
						chunkSize = fileSize
			else:
				fileData = fp.read(fileSize)
				outFile.write(crypt(context, fileData, fileSize))

			fp.close()
			outFile.write("\0" * (((file.fileSize + 0x0F) & ~0x0F) - file.fileSize))

	outFile.write('\0' * 0x60)
	outFile.close()

	os.system('cls')
	print "Package file: %s\n" % outname
	print header

def usage():
	print """usage: [based on revision 20200824]

    pkg_custom [package-file-to-extract]

    pkg_custom [target-directory] [output-file]

    pkg_custom [options] [target-directory]
        -c | --contentid        make package using content id

    pkg_custom [options] npdrm-package
        -l | --list             list packaged files
        -x | --extract          extract package

    pkg_custom [options]
        --version               print revision
        --help                  print this message"""
	print
	wait()

def version():
	print """pkg_custom 1.4.6"""

def main():
	global debug
	extract = False
	list = False
	contentid = None
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hx:dvl:c:", ["help", "extract=", "debug","version", "list=", "contentid="])
	except getopt.GetoptError:
		usage()
		sys.exit(2)
	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit(2)
		elif opt in ("-v", "--version"):
			version()
			sys.exit(2)
		elif opt in ("-x", "--extract"):
			fileToExtract = arg
			extract = True
		elif opt in ("-l", "--list"):
			fileToList = arg
			list = True
		elif opt in ("-d", "--debug"):
			debug = True
		elif opt in ("-c", "--contentid"):
			contentid = arg
		else:
			usage()
			sys.exit(2)
	if extract:
		unpack(fileToExtract)
	elif list:
		listPkg(fileToList)
	else:
		if len(args) == 1 and contentid != None:
			pack(args[0], contentid)
		elif len(args) == 2 and contentid != None:
			pack(args[0], contentid, args[1])
		elif len(args) >= 1 and len(args) <= 2 and contentid == None:
			if os.path.isdir(args[0]):
				contentid = None
				if len(args[0]) >= 36:
					print args[0]
					contentid = re.match("[0-9|A-Z]{6}-[0-9|A-Z]{9}_[0-9]{2}-[0-9|A-Z]{16}",args[0][-36:])
					if contentid == None and len(args) == 2:
						contentid = re.match("[0-9|A-Z]{6}-[0-9|A-Z]{9}_[0-9]{2}-[0-9|A-Z]{16}",args[1][-36:])
					if contentid != None and len(contentid.string) >= 36:
						if len(args) == 2:
							pack(args[0], contentid.string[:36], args[1])
						else:
							pack(args[0], contentid.string[:36])
				if contentid == None and len(args[0]) >= 9:
					titleid = re.match("([0-9|A-Z]{9})$", args[0][-9:])
					if len(args) == 1 and titleid != None:
						pack(args[0], "CUSTOM-" + titleid.string + "_00-0000000000000000", "CUSTOM-" + titleid.string + "_00-0000000000000000.pkg")
					elif len(args) == 2 and titleid != None:
						pack(args[0], "CUSTOM-" + titleid.string + "_00-0000000000000000", args[1])
					elif len(args) == 2:
						pack(args[0], "CUSTOM-INSTALLER_00-0000000000000000", args[1])
					else:
						pack(args[0], "CUSTOM-INSTALLER_00-0000000000000000")
			else:
				unpack(args[0])
		elif len(args) == 0 and os.path.isdir("custom"):
			pack("custom", "CUSTOM-INSTALLER_00-0000000000000000")
		else:
			usage()
			sys.exit(2)
if __name__ == "__main__":
	main()
