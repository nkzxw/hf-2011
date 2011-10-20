#ifndef _WEHNTRUST_REG_H
#define _WEHNTRUST_REG_H

NTSTATUS RegSetValueLong(
		IN HANDLE Key,
		IN PWSTR ValueName,
		IN ULONG ValueData);
NTSTATUS RegQueryValueLong(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PULONG ValueData);
NTSTATUS RegQueryValueBinary(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PUCHAR ValueData,
		OUT PULONG ValueSize);

NTSTATUS RegSetValueSz(
		IN HANDLE Key,
		IN PWSTR ValueName,
		IN PUNICODE_STRING UnicodeString);
NTSTATUS RegQueryValueSz(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PUNICODE_STRING UnicodeString);

NTSTATUS RegDeleteValue(
		IN HANDLE Key,
		IN PWSTR ValueName);

#endif
