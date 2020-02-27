# DnsResolver
C# async wrapper for DnsQueryEx using C++ to call native method.

## Example
```csharp
/// <summary>
/// Callback function for <see cref="ResolveDnsName(string, DnsRecordTypes, CallbackDelegate)"/>.
/// </summary>
/// <param name="status">DNS query result code.</param>
/// <param name="ipAddress">Retrieved IP Address.</param>
/// <param name="ttl">Time to live of retrieved IP Address.</param>
/// <remarks>
/// LONG is the same size as int in Windows.
/// </remarks>
[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate void CallbackDelegate(int status, string ipAddress, uint ttl);

public enum DnsRecordTypes: ushort
{
    DNS_TYPE_A = 0x0001, // IPv4 record
    DNS_TYPE_AAAA = 0x001c, // IPv6 record
}

static async Task Main(string[] args)
{
    var result = await GetHostAddressAsync("www.bing.com").ConfigureAwait(false);
    Console.WriteLine($"Result: {result}");
    Console.ReadKey();
}

static async Task<Tuple<string, uint>> GetHostAddressAsync(string hostname, DnsRecordTypes type = DnsRecordTypes.DNS_TYPE_A)
{
    var taskWrapper = new TaskCompletionSource<Tuple<string, uint>>();
    var callback = new CallbackDelegate(
        (status, ipAddress, ttl) =>
        {
            if (status != 0)
            {
                taskWrapper.SetException(new Exception($"DNS query for {hostname} failed. ErrorCode: {status}"));
                return;
            }
            
            taskWrapper.SetResult(Tuple.Create(ipAddress, ttl));
        });

    ResolveDnsName(hostname, type, callback);
    return await taskWrapper.Task;
}

/// <summary>
/// Resolves the given DNS name using native WIN32 DnsQueryEx method.
/// </summary>
/// <param name="hostname">DNS name to resolve.</param>
/// <param name="type">Type of record to retrieve.</param>
/// <param name="callback">Function to callback when resolution is complete.</param>
/// <remarks>
/// You can get the entry point by using VS Command Prompt.
/// `dumpbin /exports DnsResolver.dll`.
/// For x86 use EntryPoint = @"_ResolveDnsName".
/// </remarks>
[DllImport("DnsNameResolver.dll", EntryPoint = @"ResolveDnsName", CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
internal static extern void ResolveDnsName(string hostname, DnsRecordTypes type, CallbackDelegate callback);
```
