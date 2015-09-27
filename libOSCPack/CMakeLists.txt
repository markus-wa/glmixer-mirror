cmake_minimum_required(VERSION 2.8)
PROJECT(LIBOSCPACK)

ADD_LIBRARY(OSCPack
            OscTypes.h
            OscTypes.cpp 
            OscHostEndianness.h
            OscException.h
            OscPacketListener.h
            MessageMappingOscPacketListener.h
            OscReceivedElements.h
            OscReceivedElements.cpp
            OscPrintReceivedElements.h
            OscPrintReceivedElements.cpp
            OscOutboundPacketStream.h
            OscOutboundPacketStream.cpp
)


