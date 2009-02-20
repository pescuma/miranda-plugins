package org.pescuma.miricogen;

import static org.pescuma.miricogen.FileUtils.*;

import java.awt.image.BufferedImage;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Properties;

import net.sf.image4j.codec.ico.ICODecoder;
import net.sf.image4j.codec.ico.ICOEncoder;

import org.apache.tools.ant.taskdefs.Copy;
import org.apache.tools.ant.taskdefs.ExecTask;
import org.apache.velocity.Template;
import org.apache.velocity.VelocityContext;
import org.apache.velocity.app.Velocity;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.pescuma.jfg.gui.swt.JfgFormComposite;
import org.pescuma.jfg.gui.swt.JfgFormData;
import org.pescuma.jfg.reflect.ReflectionGroup;
import org.pescuma.velocity.VelocityLogger;
import org.pescuma.velocity.VelocityResourceLoader;

public class Main
{
	private static Config cfg;
	private static Text out;
	
	public static class Paths
	{
		public File protosPath = new File("protos");
		public File overlaysPath = new File("overlays");
		public File outPath = new File("out");
	}
	
	public static class DllInfo
	{
		public String name = "Miranda Icon Pack";
		public String description = "Miranda Icon Pack for %proto%";
		@SuppressWarnings("deprecation")
		public String copyright = "Copyright © Author Name, " + (new Date().getYear() + 1900);
		public String version = "1.0.0.0";
	}
	
	public static class Config
	{
		public final Paths paths = new Paths();
		public final DllInfo dllInfo = new DllInfo();
	}
	
	public static void main(String[] args)
	{
		initVelocity();
		
		cfg = new Config();
		Display display = new Display();
		final Shell shell = new Shell(display);
		shell.setLayout(new GridLayout(1, true));
		
		final JfgFormComposite form = new JfgFormComposite(shell, SWT.NONE, new JfgFormData(JfgFormData.DIALOG));
		form.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		form.addContentsFrom(new ReflectionGroup(cfg));
		
		Button ok = new Button(shell, SWT.PUSH);
		ok.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		ok.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				form.copyToModel();
				
				try
				{
					process();
				}
				catch (Exception e)
				{
					e.printStackTrace();
					MessageBox mb = new MessageBox(shell, SWT.OK | SWT.ERROR);
					mb.setText("Error");
					mb.setMessage(e.getMessage());
					mb.open();
				}
			}
		});
		ok.setText("Generate");
		
		out = new Text(shell, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		out.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		shell.setText("Miranda Ico Generator Config");
		shell.setDefaultButton(ok);
		shell.pack();
		shell.setSize(500, 500);
		shell.open();
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
		display.dispose();
	}
	
	protected static void process() throws IOException, InterruptedException
	{
		out.append("Starting process...\n\n");
		
		List<String> protos = readProtos();
		List<Overlay> overlays = readOverlays();
		
		for (String proto : protos)
		{
			out.append(proto + "\n");
			
			for (Overlay overlay : overlays)
			{
				out.append("   - " + overlay.getName() + " : ");
				
				File outIco = getOutIco(proto, overlay);
				
				File orig = getProtoIco(proto + "-" + overlay.getName());
				if (orig.exists())
				{
					out.append("found in prots folder\n");
					copy(outIco, orig);
					continue;
				}
				
				File protoIco = getProtoIco(proto);
				if (!protoIco.exists())
				{
					out.append("could not find " + getFullPath(protoIco) + "\n");
					continue;
				}
				
				if (overlay.isEmpty())
				{
					out.append("could not find overlay, using original icon\n");
					copy(outIco, protoIco);
					continue;
				}
				
				out.append("merging icons\n");
				createIco(outIco, protoIco, overlay);
			}
			
			out.append("   - Building dll\n");
			buildDll(proto, overlays);
			
			out.append("\n");
		}
		
		out.append("Done");
	}
	
	private static File getOutIco(String proto, Overlay overlay)
	{
		return new File(cfg.paths.outPath, proto + "-" + overlay.getName() + ".ico");
	}
	
	private static void createIco(File outIco, File protoIco, Overlay overlay) throws IOException
	{
		List<BufferedImage> protoImgs = ICODecoder.read(protoIco);
		
		for (BufferedImage proto : protoImgs)
		{
			if (proto.getWidth() == 16 && proto.getHeight() == 16)
				overlay.blend(proto);
		}
		
		if (outIco.exists())
			outIco.delete();
		ICOEncoder.write(protoImgs, outIco);
	}
	
	private static void copy(File dest, File orig) throws IOException
	{
		if (!orig.exists())
			throw new FileNotFoundException(getFullPath(orig));
		if (dest.exists())
			dest.delete();
		
		Copy copy = new Copy();
		copy.setFile(orig);
		copy.setTofile(dest);
		copy.execute();
	}
	
	private static File getProtoIco(String name)
	{
		return new File(cfg.paths.protosPath, name + ".ico");
	}
	
	private static List<Overlay> readOverlays()
	{
		List<Overlay> ret = new ArrayList<Overlay>();
		ret.add(new Overlay(cfg, "offline"));
		ret.add(new Overlay(cfg, "online"));
		ret.add(new Overlay(cfg, "away"));
		ret.add(new Overlay(cfg, "dnd"));
		ret.add(new Overlay(cfg, "na"));
		ret.add(new Overlay(cfg, "occupied"));
		ret.add(new Overlay(cfg, "freechat"));
		ret.add(new Overlay(cfg, "invisible"));
		ret.add(new Overlay(cfg, "onthephone"));
		ret.add(new Overlay(cfg, "outtolunch"));
		return ret;
	}
	
	private static List<String> readProtos()
	{
		File[] files = cfg.paths.protosPath.listFiles(new FileFilter() {
			public boolean accept(File pathname)
			{
				return pathname.getName().endsWith(".ico");
			}
		});
		List<String> ret = new ArrayList<String>();
		for (int i = 0; i < files.length; i++)
		{
			File f = files[i];
			String name = f.getName();
			if (name.indexOf('-') >= 0)
				continue;
			ret.add(name.substring(0, name.length() - 4));
		}
		return ret;
	}
	
	private static void initVelocity()
	{
		try
		{
			Properties props = new Properties();
			props.put("runtime.log.logsystem.class", VelocityLogger.class.getName());
			props.put("file.resource.loader.class", VelocityResourceLoader.class.getName());
			props.put("velocimacro.permissions.allow.inline.local.scope", "true");
			Velocity.init(props);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			throw new RuntimeException(e);
		}
	}
	
	private static void buildDll(String proto, List<Overlay> overlays) throws IOException, InterruptedException
	{
		File dllFile = new File(cfg.paths.outPath, proto + ".dll");
		if (dllFile.exists())
			dllFile.delete();
		
		File rcFile = new File(cfg.paths.outPath, proto + ".rc");
		
		mergeTemplate(proto, overlays, rcFile);
		
		String rc = getFullPath(rcFile);
		rc = rc.substring(0, rc.length() - 3);
		
		File utils = new File("utils");
		File buildme = new File(utils, "buildme.bat");
		if (!buildme.exists())
			throw new IllegalStateException("Could not find utils dir");
		
		ExecTask exec = new ExecTask();
		exec.setExecutable(getFullPath(buildme));
		exec.createArg().setValue(rc);
		exec.setDir(utils);
		exec.setFailonerror(true);
		exec.setLogError(true);
		exec.execute();
	}
	
	private static void mergeTemplate(String proto, List<Overlay> overlays, File file)
	{
		File templateName = new File("utils/autobuild.rc.vm");
		try
		{
			VelocityContext context = new VelocityContext();
			context.put("proto", encode(proto));
			context.put("description", encode(cfg.dllInfo.description.replace("%proto%", proto)));
			context.put("name", encode(cfg.dllInfo.name.replace("%proto%", proto)));
			context.put("copyright", encode(cfg.dllInfo.copyright.replace("%proto%", proto)));
			
			String[] version = cfg.dllInfo.version.split("\\.");
			for (int j = 0; j < 4; j++)
				context.put("v" + (j + 1), encode(Integer.toString(version.length >= j ? toInt(version[j]) : 0)));
			
			for (Overlay overlay : overlays)
				context.put(overlay.getName(), encode(getFullPath(getOutIco(proto, overlay))));
			
			Template template = Velocity.getTemplate(getFullPath(templateName));
			
			File parentFile = file.getParentFile();
			if (!parentFile.exists() && !parentFile.mkdirs())
				throw new IllegalStateException("Could not create the output path");
			
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "ISO-8859-1"));
			try
			{
				template.merge(context, writer);
			}
			finally
			{
				writer.close();
			}
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
	}
	
	private static int toInt(String str)
	{
		try
		{
			return Integer.parseInt(str);
		}
		catch (NumberFormatException e)
		{
			return 0;
		}
	}
	
	private static String encode(String str)
	{
		return str.replace("\\", "\\\\").replace("\"", "\\\\\"");
	}
}
