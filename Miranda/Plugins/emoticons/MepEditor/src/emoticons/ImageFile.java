package emoticons;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import org.apache.commons.httpclient.HttpClient;
import org.apache.commons.httpclient.HttpException;
import org.apache.commons.httpclient.HttpStatus;
import org.apache.commons.httpclient.methods.GetMethod;
import org.eclipse.swt.graphics.Image;

public class ImageFile
{
	public String path;
	public File realPath;
	public Image[] frames;
	public int frame;
	
	public ImageFile(String path, File realPath)
	{
		this.path = path;
		this.realPath = realPath;
	}
	
	public void loadFrames()
	{
		if (frames != null)
			return;
		
		if (path == null || realPath == null)
			return;
		
		if (haveToDownload())
			downloadFile();
		
		if (realPath == null || !realPath.exists())
			return;
		
		frames = ImageUtils.getFrames(realPath);
	}
	
	public boolean haveToDownload()
	{
		return path.startsWith("http://") && !realPath.exists();
	}
	
	private void downloadFile()
	{
		System.out.print("Going to download " + path + "...");
		
		// Create an instance of HttpClient.
		HttpClient client = new HttpClient();
		
		// Create a method instance.
		GetMethod method = new GetMethod(path);
		
		// Provide custom retry handler is necessary
		//		method.getParams().setParameter(HttpMethodParams.RETRY_HANDLER, new DefaultHttpMethodRetryHandler(3, false));
		
		try
		{
			// Execute the method.
			int statusCode = client.executeMethod(method);
			if (statusCode != HttpStatus.SC_OK)
				throw new HttpException();
			
			// Read the response body.
			byte[] responseBody = method.getResponseBody();
			
			realPath.getParentFile().mkdirs();
			
			FileOutputStream out = new FileOutputStream(realPath, false);
			out.write(responseBody);
			
			System.out.println("Done");
		}
		catch (HttpException e)
		{
			System.out.println("Err");
			realPath = null;
		}
		catch (IOException e)
		{
			System.out.println("Err");
			realPath = null;
		}
		finally
		{
			// Release the connection.
			method.releaseConnection();
		}
	}
	
}
