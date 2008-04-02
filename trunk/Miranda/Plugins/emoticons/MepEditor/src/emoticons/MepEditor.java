package emoticons;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

public class MepEditor
{
	private static MepFormat mepFormat;
	private static Mep mep;
	private static final List<String> protocols = new ArrayList<String>();
	static Shell shell;
	private static ScrolledComposite emoticonsScroll;
	private static Composite emoticonsComposite;
	
	private static boolean inGuiSetCommand;
	
	public static void main(String[] args) throws IOException
	{
		List<Emoticon> emoticons = new ArrayList<Emoticon>();
		new EmoFormat(emoticons, protocols).load();
		
		Display display = new Display();
		shell = new Shell(display);
		DirectoryDialog dialog = new DirectoryDialog(shell, SWT.NONE);
		dialog.setText("Select folder with emoticon images");
		String path = dialog.open();
		if (path == null)
			return;
		//		String path = "C:\\Desenvolvimento\\Miranda\\bin\\Debug Unicode\\Customize\\Emoticons\\Originals";
		File mepPath = new File(path);
		if (!mepPath.exists())
			return;
		
		mepFormat = new MepFormat(mepPath, emoticons, protocols);
		mep = mepFormat.load();
		
		shell.setLayout(new FillLayout());
		
		createMainWindow(shell);
		
		shell.setText("Emoticon Pack Editor");
		shell.setImage(new Image(Display.getCurrent(), "data/Defaults/smile.png"));
		shell.setSize(500, 600);
		shell.open();
		
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
		display.dispose();
	}
	
	static long time = System.nanoTime();
	
	@SuppressWarnings("unused")
	private static void profile(String text)
	{
		time = System.nanoTime() - time;
		
		System.out.println(text + " : " + (time / 1000000));
		
		time = System.nanoTime();
	}
	
	private static void createMainWindow(Composite main)
	{
		main.setLayout(new GridLayout(1, true));
		
		createToolbar(main);
		createPackGroup(main);
		createEmoticonsGroup(main, null);
	}
	
	private static void createToolbar(Composite main)
	{
		Button save = new Button(main, SWT.PUSH);
		save.setImage(new Image(Display.getCurrent(), "imgs/disk.png"));
		save.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				try
				{
					mepFormat.save();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		});
	}
	
	private static void createEmoticonsGroup(final Composite main, String protocol)
	{
		final Group group = new Group(main, SWT.NONE);
		group.setLayoutData(new GridData(GridData.FILL_BOTH));
		group.setText("Emoticons");
		group.setLayout(new FillLayout());
		
		emoticonsScroll = new ScrolledComposite(group, SWT.V_SCROLL);
		emoticonsComposite = new Composite(emoticonsScroll, SWT.NONE);
		emoticonsComposite.setLayout(new GridLayout(2, false));
		
		createBoldLabel(emoticonsComposite, "Protocol:");
		createProtoCombo(emoticonsComposite, protocol, main, group);
		
		for (final Emoticon emo : mep.emoticons)
		{
			EmoticonImage icon = emo.icons.get(protocol);
			if (icon == null)
				continue;
			
			createEmoticonNameLabel(emoticonsComposite, emo);
			
			Composite attribs = new Composite(emoticonsComposite, SWT.NONE);
			attribs.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
			GridLayout gl = new GridLayout(2, false);
			gl.marginWidth = 0;
			attribs.setLayout(gl);
			
			createIconCombo(icon, attribs, emo, protocol);
			createFramesHolder(emo, icon, attribs);
		}
		
		emoticonsScroll.setContent(emoticonsComposite);
		emoticonsScroll.setExpandHorizontal(true);
		emoticonsScroll.setExpandVertical(true);
		emoticonsScroll.addControlListener(new ControlAdapter() {
			@Override
			public void controlResized(ControlEvent e)
			{
				Rectangle r = emoticonsScroll.getClientArea();
				emoticonsScroll.setMinSize(emoticonsComposite.computeSize(r.width, SWT.DEFAULT));
			}
		});
		
		emoticonsComposite.pack();
		emoticonsScroll.setMinSize(emoticonsComposite.computeSize(SWT.DEFAULT, SWT.DEFAULT, false));
	}
	
	private static void createEmoticonNameLabel(final Composite parent, final Emoticon emo)
	{
		Composite nameComposite = new Composite(parent, SWT.NONE);
		nameComposite.setLayoutData(new GridData());
		nameComposite.setLayout(new GridLayout(2, false));
		
		File defImage = new File("data/Defaults/" + emo.name + ".png");
		if (defImage.exists())
		{
			try
			{
				Image image = new Image(Display.getCurrent(), defImage.getCanonicalPath());
				Label label = new Label(nameComposite, SWT.NONE);
				label.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
				label.setImage(image);
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
		
		createBoldLabel(nameComposite, emo.name);
	}
	
	private static void createProtoCombo(final Composite parent, String protocol, final Composite main, final Group group)
	{
		final Combo protoCombo = new Combo(parent, SWT.READ_ONLY);
		protoCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		protoCombo.add("All protocols");
		for (String p : protocols)
			protoCombo.add(p);
		protoCombo.select(protocol == null ? 0 : protoCombo.indexOf(protocol));
		protoCombo.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event e)
			{
				runAllDelayedListeners();
				
				String proto = protoCombo.getText();
				if (!protocols.contains(proto))
					proto = null;
				
				group.dispose();
				
				createEmoticonsGroup(main, proto);
				
				shell.layout();
			}
		});
	}
	
	private static void createIconCombo(final EmoticonImage icon, Composite parent, final Emoticon emo, final String protocol)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setText("Icon:");
		
		final Combo combo = new Combo(parent, SWT.NONE);
		combo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		
		if (icon.image != null)
			combo.setText(icon.image.path);
		
		combo.addListener(SWT.FocusIn, new Listener() {
			public void handleEvent(Event event)
			{
				if (combo.getItemCount() > 0)
					return;
				
				inGuiSetCommand = true;
				
				for (ImageFile file : mep.images)
					combo.add(file.path);
				
				inGuiSetCommand = false;
			}
		});
		
		DelayedListener listener = new DelayedListener(new Listener() {
			public void handleEvent(Event e)
			{
				icon.image = mep.createTemporaryImage(combo.getText(), protocol);
				fillFramesHolder(emo, icon);
			}
		});
		combo.addListener(SWT.Selection, listener.fast());
		combo.addListener(SWT.Modify, listener.delayed());
	}
	
	private static final Set<DelayedListener> delayedListeners = new HashSet<DelayedListener>();
	
	private static void runAllDelayedListeners()
	{
		while (delayedListeners.size() > 0)
		{
			delayedListeners.iterator().next().run();
		}
	}
	
	static class DelayedListener
	{
		private Event event;
		private Listener listener;
		private Color bkg;
		private Runnable run = new Runnable() {
			public void run()
			{
				DelayedListener.this.run();
			}
		};
		
		public DelayedListener(Listener listener)
		{
			this.listener = listener;
		}
		
		private void run()
		{
			Display.getCurrent().timerExec(-1, run);
			delayedListeners.remove(this);
			((Control) event.widget).setBackground(bkg);
			listener.handleEvent(event);
		}
		
		private void start(Event e)
		{
			event = e;
			
			if (bkg == null)
				bkg = ((Control) e.widget).getBackground();
			((Control) e.widget).setBackground(new Color(Display.getCurrent(), 255, 255, 200));
			
			delayedListeners.add(this);
			Display.getCurrent().timerExec(-1, run);
			Display.getCurrent().timerExec(3000, run);
		}
		
		public Listener delayed()
		{
			return new Listener() {
				public void handleEvent(Event e)
				{
					if (inGuiSetCommand)
						return;
					
					start(e);
				}
			};
		}
		
		public Listener fast()
		{
			return new Listener() {
				public void handleEvent(Event e)
				{
					if (inGuiSetCommand)
						return;
					
					event = e;
					run();
				}
			};
		}
	};
	
	private static void fillFramesHolder(final Emoticon emo, final EmoticonImage icon)
	{
		Display.getCurrent().asyncExec(new Runnable() {
			public void run()
			{
				disposeAllChildren(emo.frames);
				
				if (icon.image == null)
					return;
				
				icon.image.loadFrames();
				if (icon.image.frames == null)
					return;
				
				Listener listener = new Listener() {
					public void handleEvent(Event e)
					{
						Button button = (Button) e.widget;
						
						Control[] children = emo.frames.getChildren();
						for (int i = 0; i < children.length; i++)
						{
							Control child = children[i];
							if (e.widget != child && child instanceof Button && (child.getStyle() & SWT.TOGGLE) != 0)
							{
								((Button) child).setSelection(false);
							}
							else if (child == button)
							{
								icon.image.frame = i;
							}
						}
						button.setSelection(true);
					}
				};
				
				Image[] frames = icon.image.frames;
				for (int j = 0; j < frames.length; j++)
				{
					Button button = new Button(emo.frames, SWT.TOGGLE);
					button.setImage(frames[j]);
					if (j == icon.image.frame)
						button.setSelection(true);
					button.addListener(SWT.Selection, listener);
				}
				
				emo.frames.layout();
				emoticonsComposite.pack();
				Rectangle r = emoticonsScroll.getClientArea();
				emoticonsScroll.setMinSize(emoticonsComposite.computeSize(r.width, SWT.DEFAULT));
			}
		});
	}
	
	private static void disposeAllChildren(final Composite composite)
	{
		Control[] children = composite.getChildren();
		for (int i = 0; i < children.length; i++)
			children[i].dispose();
	}
	
	private static void createFramesHolder(Emoticon emo, EmoticonImage icon, Composite parent)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setText("Frame:");
		
		emo.frames = new Composite(parent, SWT.NONE);
		emo.frames.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		RowLayout rowLayout = new RowLayout(SWT.HORIZONTAL);
		rowLayout.wrap = true;
		emo.frames.setLayout(rowLayout);
		
		fillFramesHolder(emo, icon);
	}
	
	private static void createPackGroup(Composite main)
	{
		Group group = new Group(main, SWT.NONE);
		group.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		group.setText("Pack");
		group.setLayout(new GridLayout(2, false));
		
		createBoldLabel(group, "Name:");
		Text text = new Text(group, SWT.BORDER);
		text.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		text.setText(mep.name);
		text.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e)
			{
				mep.name = ((Text) e.widget).getText();
			}
		});
		
		createBoldLabel(group, "Creator:");
		text = new Text(group, SWT.BORDER);
		text.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		text.setText(mep.creator);
		text.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e)
			{
				mep.creator = ((Text) e.widget).getText();
			}
		});
		
		createBoldLabel(group, "Updater URL:");
		text = new Text(group, SWT.BORDER);
		text.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		text.setText(mep.udaterURL);
		text.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e)
			{
				mep.udaterURL = ((Text) e.widget).getText();
			}
		});
		
	}
	
	private static void createBoldLabel(Composite parent, String text)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setLayoutData(new GridData());
		label.setText(text);
		setBoldFont(label);
	}
	
	private static void setBoldFont(Label name)
	{
		Font initialFont = name.getFont();
		FontData[] fontData = initialFont.getFontData();
		for (int i = 0; i < fontData.length; i++)
			fontData[i].setStyle(SWT.BOLD);
		Font newFont = new Font(Display.getCurrent(), fontData);
		name.setFont(newFont);
	}
	
}